//
//  RNJoltDynamicBody.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltDynamicBody.h"
#include "RNJoltRigidBodyController.h"
#include "RNJoltInternals.h"
#include <Jolt/Physics/Character/Character.h>
#include "RNJoltWorld.h"
#include <Jolt/Physics/Body/AllowedDOFs.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionProperties.h>
#include <Jolt/Physics/Constraints/TwoBodyConstraint.h>

namespace RN
{
	RNDefineMeta(JoltDynamicBody, JoltCollisionObject)

	JoltDynamicBody::JoltDynamicBody(JoltShape *shape, float mass, JoltWorld *owningWorld) :
		JoltCollisionObject(owningWorld),
		_shape(shape->Retain()),
		_actor(nullptr), _mass(mass), _appliedShapeCenterOfMass(shape->GetCenterOfMass()), _isKinematic(false), _isGravityEnabled(true), _isInSimulation(false)
	{
		JoltWorld *world = GetJoltWorld();
		JPH::PhysicsSystem *physics = world->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();

		JPH::BodyCreationSettings settings(shape->GetJoltShape(), JPH::RVec3::sZero(), JPH::QuatArg(0.0f, 0.0f, 0.0f, 1.0f), JPH::EMotionType::Dynamic, world->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1));
		settings.mLinearDamping = world->GetDefaultDynamicBodyLinearDamping();
		settings.mAngularDamping = world->GetDefaultDynamicBodyAngularDamping();
		settings.mMaxLinearVelocity = world->GetDefaultDynamicBodyMaxLinearVelocity();
		settings.mMaxAngularVelocity = world->GetDefaultDynamicBodyMaxAngularVelocity();
		settings.mMassPropertiesOverride.mMass = mass;
		settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		settings.mMotionQuality = JPH::EMotionQuality::LinearCast;
		settings.mEnhancedInternalEdgeRemoval = true;
		settings.mUserData = reinterpret_cast<uint64>(this);
		JPH::BodyID bodyID = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::DontActivate);

		if(!bodyID.IsInvalid())
		{
			_actor = new JPH::BodyID();
			*_actor = bodyID;
			_isInSimulation = true;
		}
	}

	JoltDynamicBody::~JoltDynamicBody()
	{
		JoltWorld *world = GetJoltWorld();
		JPH::PhysicsSystem *physics = world->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();

		if(_actor)
		{
			world->RemoveConstraintsForBody(*_actor);
			world->CancelQueuedBodyRemoval(*_actor);
			if(bodyInterface.IsAdded(*_actor))
			{
				bodyInterface.DeactivateBody(*_actor);
				bodyInterface.RemoveBody(*_actor);
			}
			bodyInterface.DestroyBody(*_actor);
		}

		if(_actor) delete _actor;
		_shape->Release();
	}

	bool JoltDynamicBody::TransferToWorld(const std::vector<JoltDynamicBody *> &bodies, JoltWorld *destination,
		const std::function<bool(const JPH::Constraint *)> &canRemoveConstraint, const std::function<void()> &beforeTransfer, const std::function<void()> &afterTransfer)
	{
		return TransferToWorld(bodies, {}, destination, canRemoveConstraint, beforeTransfer, afterTransfer);
	}

	bool JoltDynamicBody::TransferToWorld(const std::vector<JoltDynamicBody *> &bodies, const std::vector<JoltRigidBodyController *> &controllers, JoltWorld *destination,
		const std::function<bool(const JPH::Constraint *)> &canRemoveConstraint, const std::function<void()> &beforeTransfer, const std::function<void()> &afterTransfer)
	{
		if(!destination || bodies.empty() || !bodies.front()) return false;
		JoltWorld *source = bodies.front()->GetJoltWorld();
		if(!source || source == destination) return false;
		auto *sourcePhysics = source->GetJoltInstance();
		auto &sourceInterface = sourcePhysics->GetBodyInterface();
		auto &destinationInterface = destination->GetJoltInstance()->GetBodyInterface();
		for(size_t index = 0; index < bodies.size(); ++index)
		{
			auto *body = bodies[index];
			if(!body || body->GetJoltWorld() != source || !body->_actor) return false;
			for(size_t previous = 0; previous < index; ++previous) if(bodies[previous] == body) return false;
		}
		for(size_t index = 0; index < controllers.size(); ++index)
		{
			auto *controller = controllers[index];
			if(!controller || controller->GetJoltWorld() != source || !controller->_controller || controller->_controller->GetBodyID().IsInvalid()) return false;
			for(size_t previous = 0; previous < index; ++previous) if(controllers[previous] == controller) return false;
			if(controller->_externalSupportAnchorValid)
			{
				bool internal = false;
				for(auto *body : bodies) if(body->GetJoltBodyID() == controller->_externalSupportBodyID) internal = true;
				if(!internal) return false;
			}
		}
		auto containsBody = [&](const JPH::Body *body) {
			for(auto *controller : controllers) if(body->GetID() == controller->_controller->GetBodyID()) return true;
			for(auto *attachment : bodies) if(body->GetID() == *attachment->_actor) return true;
			return false;
		};
		for(const auto &constraint : sourcePhysics->GetConstraints())
		{
			if(constraint->GetType() == JPH::EConstraintType::TwoBodyConstraint)
			{
				const auto *joint = static_cast<const JPH::TwoBodyConstraint *>(constraint.GetPtr());
				if(!containsBody(joint->GetBody1()) && !containsBody(joint->GetBody2())) continue;
				if(!containsBody(joint->GetBody1()) || !containsBody(joint->GetBody2())) return false;
			}
			bool controllerSupport = false;
			for(auto *controller : controllers) if(controller->_externalSupportConstraint == constraint.GetPtr()) controllerSupport = true;
			if(controllerSupport) continue;
			// Opaque multi-body constraints must be explicitly accepted by their owner.
			if(!canRemoveConstraint || !canRemoveConstraint(constraint.GetPtr())) return false;
		}
		struct PreparedBody
		{
			JPH::Body *body;
			bool active;
			JPH::BodyCreationSettings settings;
			JPH::Vec3 force, torque;
		};
		std::vector<PreparedBody> prepared;
		struct PreparedController
		{
			JPH::Character *character;
			PreparedBody body;
			bool added;
			uint32 supportIndex;
			Vector3 supportPosition;
			float supportForce;
		};
		std::vector<PreparedController> preparedControllers;
		auto discard = [&]() {
			for(const auto &entry : prepared) destinationInterface.DestroyBody(entry.body->GetID());
			for(const auto &entry : preparedControllers) delete entry.character;
		};
		auto convertVector = [&](JPH::Vec3Arg vector) {
			return JoltNativeConversions::ToJoltVector(JoltConversions::ConvertVectorToPhysicsWorld(JoltConversions::ConvertVectorFromPhysicsWorld(JoltNativeConversions::ToEngineVector(vector), source), destination));
		};
		auto convertSettings = [&](JPH::BodyCreationSettings &settings, const JPH::Body &body, bool controller) {
			const PositionType centerOfMass = JoltConversions::ConvertPositionFromPhysicsWorld(JoltNativeConversions::ToPosition(body.GetCenterOfMassPosition()), source);
			settings.mPosition = JoltNativeConversions::ToJoltPosition(JoltConversions::ConvertPositionToPhysicsWorld(JoltConversions::ConvertPositionFromPhysicsWorld(JoltNativeConversions::ToPosition(settings.mPosition), source), destination));
			settings.mRotation = JoltNativeConversions::ToJoltRotation(JoltConversions::ConvertRotationToPhysicsWorld(JoltConversions::ConvertRotationFromPhysicsWorld(JoltNativeConversions::ToEngineRotation(settings.mRotation), source), destination));
			settings.mLinearVelocity = JoltNativeConversions::ToJoltVector(JoltConversions::ConvertLinearVelocityToPhysicsWorld(JoltConversions::ConvertLinearVelocityFromPhysicsWorld(JoltNativeConversions::ToEngineVector(settings.mLinearVelocity), centerOfMass, source), centerOfMass, destination));
			// Character orientation is application-driven with angular DOFs locked.
			settings.mAngularVelocity = controller ? convertVector(settings.mAngularVelocity) : JoltNativeConversions::ToJoltVector(JoltConversions::ConvertAngularVelocityToPhysicsWorld(JoltConversions::ConvertAngularVelocityFromPhysicsWorld(JoltNativeConversions::ToEngineVector(settings.mAngularVelocity), source), destination));
			if(body.IsStatic() && (settings.mLinearVelocity.LengthSq() != 0.0f || settings.mAngularVelocity.LengthSq() != 0.0f)) return false;
			if(const auto *motion = body.GetMotionPropertiesUnchecked())
			{
				if(settings.mLinearVelocity.Length() > settings.mMaxLinearVelocity || settings.mAngularVelocity.Length() > settings.mMaxAngularVelocity) return false;
				if((motion->LockTranslation(settings.mLinearVelocity) - settings.mLinearVelocity).LengthSq() > 1.0e-12f || (motion->LockAngular(settings.mAngularVelocity) - settings.mAngularVelocity).LengthSq() > 1.0e-12f) return false;
			}
			else if(settings.mLinearVelocity.LengthSq() != 0.0f || settings.mAngularVelocity.LengthSq() != 0.0f) return false;
			return JoltNativeConversions::ToPosition(settings.mPosition).IsValid() && JoltNativeConversions::ToEngineRotation(settings.mRotation).IsValid() && JoltNativeConversions::ToEngineVector(settings.mLinearVelocity).IsValid() && JoltNativeConversions::ToEngineVector(settings.mAngularVelocity).IsValid();
		};
		auto shouldActivate = [](const JPH::Body &body, const JPH::BodyCreationSettings &settings) {
			return body.IsActive() || (!body.IsStatic() && (settings.mLinearVelocity != JPH::Vec3::sZero() || settings.mAngularVelocity != JPH::Vec3::sZero()));
		};
		for(auto *attachment : bodies)
		{
			JPH::BodyLockRead lock(sourcePhysics->GetBodyLockInterface(), *attachment->_actor);
			if(!lock.Succeeded()) { discard(); return false; }
			const JPH::Body &body = lock.GetBody();
			auto settings = body.GetBodyCreationSettings();
			if(!convertSettings(settings, body, false)) { discard(); return false; }
			settings.mObjectLayer = destination->GetObjectLayer(attachment->_collisionFilterGroup, attachment->_isInSimulation ? attachment->_collisionFilterMask : 0, 1);
			JPH::Body *replacement = destinationInterface.CreateBody(settings);
			if(!replacement) { discard(); return false; }
			if(body.IsDynamic())
			{
				replacement->AddForce(convertVector(body.GetAccumulatedForce()));
				replacement->AddTorque(convertVector(body.GetAccumulatedTorque()));
			}
			prepared.push_back({replacement, shouldActivate(body, settings), settings, body.IsDynamic() ? replacement->GetAccumulatedForce() : JPH::Vec3::sZero(), body.IsDynamic() ? replacement->GetAccumulatedTorque() : JPH::Vec3::sZero()});
		}
		for(auto *attachment : controllers)
		{
			auto characterSettings = attachment->_controller->GetCharacterSettings();
			JPH::BodyLockRead lock(sourcePhysics->GetBodyLockInterface(), attachment->_controller->GetBodyID());
			if(!lock.Succeeded()) { discard(); return false; }
			const auto &body = lock.GetBody();
			auto settings = body.GetBodyCreationSettings();
			if(!convertSettings(settings, body, true)) { discard(); return false; }
			characterSettings.mUp = convertVector(characterSettings.mUp);
			characterSettings.mLayer = destination->GetObjectLayer(attachment->_collisionFilterGroup, attachment->_collisionFilterMask, 1);
			auto *character = new JPH::Character(&characterSettings, settings.mPosition, settings.mRotation, settings.mUserData, destination->GetJoltInstance());
			if(character->GetBodyID().IsInvalid())
			{
				// Jolt's Character destructor unconditionally destroys its body ID.
				// With no body allocated only CharacterBase owns resources.
				character->JPH::CharacterBase::~CharacterBase();
				JPH::Character::operator delete(character);
				discard(); return false;
			}
			JPH::BodyLockWrite replacementLock(destination->GetJoltInstance()->GetBodyLockInterface(), character->GetBodyID());
			auto &replacement = replacementLock.GetBody();
			replacement.SetAllowSleeping(settings.mAllowSleeping);
			replacement.SetFriction(settings.mFriction);
			replacement.SetRestitution(settings.mRestitution);
			auto *motion = replacement.GetMotionProperties();
			motion->SetLinearDamping(settings.mLinearDamping);
			motion->SetAngularDamping(settings.mAngularDamping);
			motion->SetGravityFactor(settings.mGravityFactor);
			motion->SetNumPositionStepsOverride(settings.mNumPositionStepsOverride);
			motion->SetNumVelocityStepsOverride(settings.mNumVelocityStepsOverride);
			uint32 supportIndex = 0xffffffff;
			for(size_t index = 0; index < bodies.size(); ++index) if(bodies[index]->GetJoltBodyID() == attachment->_externalSupportBodyID) supportIndex = static_cast<uint32>(index);
			preparedControllers.push_back({character, {&replacement, shouldActivate(body, settings), settings, convertVector(body.GetAccumulatedForce()), convertVector(body.GetAccumulatedTorque())}, body.IsInBroadPhase(), supportIndex, attachment->_externalSupportLocalPosition, attachment->_externalSupportAnchorMaxForce});
		}
		if(beforeTransfer) beforeTransfer();
		for(auto *attachment : controllers) attachment->ClearExternalSupportAnchor();
		// Wrapper autoreleases can outlive their VCK owner. Invalidate any remaining
		// native joints while their old bodies still exist.
		for(auto *attachment : bodies) source->RemoveConstraintsForBody(*attachment->_actor);
		for(size_t index = 0; index < bodies.size(); ++index)
		{
			auto *attachment = bodies[index];
			source->CancelQueuedBodyRemoval(*attachment->_actor);
			if(sourceInterface.IsAdded(*attachment->_actor)) sourceInterface.RemoveBody(*attachment->_actor);
			sourceInterface.DestroyBody(*attachment->_actor);
			*attachment->_actor = prepared[index].body->GetID();
			attachment->_world = destination;
			attachment->InvalidateMotionCache();
			if(attachment->_isInSimulation)
				destinationInterface.AddBody(*attachment->_actor, prepared[index].active ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
		}
		for(size_t index = 0; index < controllers.size(); ++index)
		{
			auto *attachment = controllers[index];
			const auto &entry = preparedControllers[index];
			const auto oldID = attachment->_controller->GetBodyID();
			source->RemoveConstraintsForBody(oldID);
			source->CancelQueuedBodyRemoval(oldID);
			if(sourceInterface.IsAdded(oldID)) attachment->_controller->RemoveFromPhysicsSystem();
			delete attachment->_controller;
			attachment->_controller = entry.character;
			attachment->_world = destination;
			attachment->_upDirection = JoltNativeConversions::ToEngineVector(convertVector(JoltNativeConversions::ToJoltVector(attachment->_upDirection)));
			attachment->_objectBelow = nullptr;
			attachment->_groundVelocity = Vector3();
			attachment->_groundAngularVelocity = Vector3();
			attachment->_groundNormal = Vector3();
			attachment->_isFalling = true;
			if(entry.added) entry.character->AddToPhysicsSystem(entry.body.active ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
			if(entry.supportIndex != 0xffffffff) attachment->SetExternalSupportAnchor(bodies[entry.supportIndex]->GetJoltBodyID(), entry.supportPosition, entry.supportForce);
		}
		if(afterTransfer) afterTransfer();
		// Constraint recreation may wake bodies or clamp their velocity limits.
		auto restoreBody = [&](const PreparedBody &entry, bool added) {
			if(entry.body->IsDynamic())
			{
				entry.body->ResetForce(); entry.body->ResetTorque();
				entry.body->AddForce(entry.force); entry.body->AddTorque(entry.torque);
			}
			if(auto *motion = entry.body->GetMotionPropertiesUnchecked())
			{
				motion->SetMaxLinearVelocity(entry.settings.mMaxLinearVelocity);
				motion->SetMaxAngularVelocity(entry.settings.mMaxAngularVelocity);
				motion->SetLinearVelocity(entry.settings.mLinearVelocity);
				motion->SetAngularVelocity(entry.settings.mAngularVelocity);
			}
			if(added)
			{
				if(entry.active) destinationInterface.ActivateBody(entry.body->GetID());
				else destinationInterface.DeactivateBody(entry.body->GetID());
			}
		};
		for(size_t index = 0; index < bodies.size(); ++index)
		{
			restoreBody(prepared[index], bodies[index]->_isInSimulation);
			bodies[index]->InvalidateMotionCache();
		}
		for(const auto &entry : preparedControllers) restoreBody(entry.body, entry.added);
		return true;
	}

	void JoltDynamicBody::SetCollisionFilter(uint32 group, uint32 mask)
	{
		JoltCollisionObject::SetCollisionFilter(group, mask);
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;
		bodyInterface->SetObjectLayer(*_actor, GetJoltWorld()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1));

		/*Jolt::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;*/
	}


	JoltDynamicBody *JoltDynamicBody::WithShape(JoltShape *shape, float mass, JoltWorld *world)
	{
		JoltDynamicBody *body = new JoltDynamicBody(shape, mass, world);
		return body->Autorelease();
	}

	JPH::BodyInterface *JoltDynamicBody::GetBodyInterfaceIfInSimulation()
	{
		if(!_actor || !_isInSimulation) return nullptr;
		InvalidateMotionCache();

		JPH::BodyInterface &bodyInterface = GetJoltWorld()->GetJoltInstance()->GetBodyInterface();
		return &bodyInterface;
	}

	bool JoltDynamicBody::RefreshMotionCacheIfNeeded() const
	{
		if(!_actor) return false;
		if(_motionCacheIsValid) return true;

		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		JPH::BodyLockRead lock(physics->GetBodyLockInterface(), *_actor);
		if(!lock.Succeeded()) return false;

		const JPH::Body &body = lock.GetBody();
		_motionCacheProperties = JoltPointMotionProperties();
		_motionCacheCenterOfMass = JoltNativeConversions::ToPosition(body.GetCenterOfMassPosition());
		_motionCacheLinearVelocity = JoltNativeConversions::ToEngineVector(body.GetLinearVelocity());
		_motionCacheAngularVelocity = JoltNativeConversions::ToEngineVector(body.GetAngularVelocity());

		const JPH::MotionProperties *motionProperties = body.IsDynamic() ? body.GetMotionPropertiesUnchecked() : nullptr;
		if(motionProperties)
		{
			_motionCacheProperties.isDynamic = true;
			_motionCacheProperties.inverseMass = motionProperties->GetInverseMass();

			JPH::Mat44 inverseInertia = body.GetInverseInertia();
			_motionCacheProperties.inverseInertiaColumnX = JoltNativeConversions::ToEngineVector(inverseInertia * JPH::Vec3::sAxisX());
			_motionCacheProperties.inverseInertiaColumnY = JoltNativeConversions::ToEngineVector(inverseInertia * JPH::Vec3::sAxisY());
			_motionCacheProperties.inverseInertiaColumnZ = JoltNativeConversions::ToEngineVector(inverseInertia * JPH::Vec3::sAxisZ());
		}

		_motionCacheIsValid = true;
		return true;
	}

	void JoltDynamicBody::SetShape(JoltShape *shape, float mass)
	{
		SetShape(shape, mass, _positionOffset);
	}

	void JoltDynamicBody::SetShape(JoltShape *shape, float mass, const Vector3 &positionOffset)
	{
		if(!shape || !_actor) return;

		Quaternion worldRotation = GetWorldRotation();

		JoltShape *previousShape = _shape;
		_shape = shape->Retain();
		_positionOffset = positionOffset;

		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		bool wasInSimulation = bodyInterface.IsAdded(*_actor);
		bool shouldActivate = wasInSimulation && _isInSimulation;
		if(wasInSimulation)
		{
			bodyInterface.SetShape(*_actor, _shape->GetJoltShape(), true, shouldActivate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
		}
		else
		{
			const JPH::BodyLockInterface &lockInterface = physics->GetBodyLockInterface();
			JPH::BodyLockWrite lock(lockInterface, *_actor);
			if(lock.Succeeded())
			{
				lock.GetBody().SetShapeInternal(_shape->GetJoltShape(), true);
			}
		}

		if(previousShape) previousShape->Release();
		SetMass(mass);
		_appliedShapeCenterOfMass = _shape->GetCenterOfMass();

		if(worldRotation.IsValid() && positionOffset.IsValid())
		{
			worldRotation.Normalize();
			Quaternion bodyRotation = worldRotation * _rotationOffset;
			bodyRotation.Normalize();
			JPH::RVec3 bodyPosition = JoltNativeConversions::GetAttachmentPosition(this, -worldRotation.GetRotatedVector(_positionOffset), GetJoltWorld());
			if(wasInSimulation)
			{
				bodyInterface.SetPositionAndRotation(*_actor, bodyPosition, JoltNativeConversions::ToJoltSceneRotation(bodyRotation, GetJoltWorld()), JPH::EActivation::DontActivate);
			}
			else
			{
				const JPH::BodyLockInterface &lockInterface = physics->GetBodyLockInterface();
				JPH::BodyLockWrite transformLock(lockInterface, *_actor);
				if(transformLock.Succeeded())
				{
					transformLock.GetBody().SetPositionAndRotationInternal(bodyPosition, JoltNativeConversions::ToJoltSceneRotation(bodyRotation, GetJoltWorld()));
				}
			}
		}

		InvalidateMotionCache();
	}

	void JoltDynamicBody::SetMass(float mass)
	{
		_mass = mass;

		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		const JPH::BodyLockInterface &lockInterface = physics->GetBodyLockInterface();
		JPH::BodyLockWrite lock(lockInterface, *_actor);
		if(!lock.Succeeded()) return;
		JPH::Body &body = lock.GetBody();
		JPH::MotionProperties *mp = body.GetMotionProperties();
		if(!mp) return; // static/kinematic

		// Preserve current DOF locks
		JPH::EAllowedDOFs allowed = mp->GetAllowedDOFs();

		// Compute mass properties from shape and scale to target mass
		JPH::MassProperties props = _shape->GetJoltShape()->GetMassProperties();
		props.ScaleToMass(mass);

		// Apply new mass properties
		mp->SetMassProperties(allowed, props);
		InvalidateMotionCache();
	}

	void JoltDynamicBody::RefreshShapeMassProperties(float mass)
	{
		if(!_shape || !_actor || mass <= k::EpsilonFloat) return;

		Vector3 currentCenterOfMass = _shape->GetCenterOfMass();
		Vector3 previousCenterOfMass = _appliedShapeCenterOfMass.IsValid() ? _appliedShapeCenterOfMass : Vector3();
		Vector3 localCenterOfMassDelta = currentCenterOfMass - previousCenterOfMass;
		Vector3 linearVelocity = GetLinearVelocity();
		Vector3 angularVelocity = GetAngularVelocity();
		Quaternion worldRotation = GetWorldRotation();
		bool hasCenterOfMassDelta = localCenterOfMassDelta.IsValid() && worldRotation.IsValid();
		Vector3 centerOfMassDelta;
		if(hasCenterOfMassDelta)
		{
			worldRotation.Normalize();
			centerOfMassDelta = worldRotation.GetRotatedVector(localCenterOfMassDelta);
		}

		_mass = mass;

		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		bool wasInSimulation = bodyInterface.IsAdded(*_actor);
		bool shouldActivate = wasInSimulation && _isInSimulation;
		bodyInterface.NotifyShapeChanged(*_actor, JoltNativeConversions::ToJoltVector(previousCenterOfMass), true, shouldActivate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
		_appliedShapeCenterOfMass = currentCenterOfMass;

		if(hasCenterOfMassDelta)
		{
			SetLinearVelocity(linearVelocity + angularVelocity.GetCrossProduct(centerOfMassDelta));
		}
		InvalidateMotionCache();
		UpdatePosition();
	}

	float JoltDynamicBody::GetMass() const
	{
		return _mass;
	}

	uint32 JoltDynamicBody::GetJoltBodyID() const
	{
		return _actor ? _actor->GetIndexAndSequenceNumber() : JPH::BodyID::cInvalidBodyID;
	}

	void JoltDynamicBody::SetLinearVelocity(const Vector3 &velocity)
	{
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;

		bodyInterface->SetLinearVelocity(*_actor, JoltNativeConversions::ToJoltVector(velocity));
	}
	void JoltDynamicBody::SetAngularVelocity(const Vector3 &velocity)
	{
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;

		bodyInterface->SetAngularVelocity(*_actor, JoltNativeConversions::ToJoltVector(velocity));
	}

	void JoltDynamicBody::SetDamping(float linear, float angular)
	{
		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		const JPH::BodyLockInterface &lockInterface = physics->GetBodyLockInterface();
		JPH::BodyLockWrite lock(lockInterface, *_actor);
		if(!lock.Succeeded()) return;
		JPH::Body &body = lock.GetBody();
		JPH::MotionProperties *mp = body.GetMotionProperties();
		if(!mp) return; // static/kinematic
		mp->SetLinearDamping(linear);
		mp->SetAngularDamping(angular);
	}

	void JoltDynamicBody::SetMaxLinearVelocity(float max)
	{
		if(!_actor) return;

		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		bodyInterface.SetMaxLinearVelocity(*_actor, max);
	}

	void JoltDynamicBody::SetMaxAngularVelocity(float max)
	{
		if(!_actor) return;

		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		bodyInterface.SetMaxAngularVelocity(*_actor, max);
	}

	void JoltDynamicBody::SetMaxDepenetrationVelocity(float max)
	{
		//_actor->setMaxDepenetrationVelocity(max);
	}

	Vector3 JoltDynamicBody::GetLinearVelocity() const
	{
		return RefreshMotionCacheIfNeeded() ? _motionCacheLinearVelocity : Vector3();
	}
	Vector3 JoltDynamicBody::GetAngularVelocity() const
	{
		return RefreshMotionCacheIfNeeded() ? _motionCacheAngularVelocity : Vector3();
	}

	Vector3 JoltDynamicBody::GetPointVelocity(const JoltPosition &globalPosition) const
	{
		return GetPointMotionProperties(globalPosition).velocity;
	}

	JoltPointMotionProperties JoltDynamicBody::GetPointMotionProperties(const JoltPosition &globalPosition) const
	{
		if(!globalPosition.IsValid() || !RefreshMotionCacheIfNeeded()) return JoltPointMotionProperties();

		JoltPointMotionProperties properties = _motionCacheProperties;
		properties.centerOffset = JoltNativeConversions::ToVector3(globalPosition - _motionCacheCenterOfMass);
		properties.velocity = _motionCacheLinearVelocity + _motionCacheAngularVelocity.GetCrossProduct(properties.centerOffset);
		return properties;
	}

	JoltPosition JoltDynamicBody::GetCenterOfMassPosition() const
	{
		return (_actor && RefreshMotionCacheIfNeeded()) ? _motionCacheCenterOfMass : JoltNativeConversions::ToPosition(JoltNativeConversions::GetAttachmentPosition(this, GetJoltWorld()));
	}

	float JoltDynamicBody::GetPointImpulseEffectiveMass(const JoltPosition &globalPosition, const Vector3 &direction) const
	{
		return GetPointMotionProperties(globalPosition).GetPointImpulseEffectiveMass(direction);
	}

	float JoltDynamicBody::GetAngularImpulseEffectiveInertia(const Vector3 &axis) const
	{
		if(!axis.IsValid() || axis.GetSquaredLength() <= k::EpsilonFloat) return 0.0f;
		if(!RefreshMotionCacheIfNeeded() || !_motionCacheProperties.isDynamic) return 0.0f;

		Vector3 normalizedAxis = axis.GetNormalized();
		Vector3 angularVelocityPerImpulse = _motionCacheProperties.inverseInertiaColumnX * normalizedAxis.x + _motionCacheProperties.inverseInertiaColumnY * normalizedAxis.y + _motionCacheProperties.inverseInertiaColumnZ * normalizedAxis.z;
		float denominator = normalizedAxis.GetDotProduct(angularVelocityPerImpulse);
		return denominator > 0.0f ? 1.0f / denominator : 0.0f;
	}

	void JoltDynamicBody::SetEnableSleeping(bool sleeping)
	{
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;

		if(!sleeping)
			bodyInterface->ActivateBody(*_actor);
		else
			bodyInterface->DeactivateBody(*_actor);
	}

	void JoltDynamicBody::SetAllowSleeping(bool allow)
	{
		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		const JPH::BodyLockInterface &lockInterface = physics->GetBodyLockInterface();
		{
			JPH::BodyLockWrite lock(lockInterface, *_actor);
			if(!lock.Succeeded()) return;

			JPH::Body &body = lock.GetBody();
			body.SetAllowSleeping(allow);
		}
		if(!allow)
		{
			JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
			if(bodyInterface) bodyInterface->ActivateBody(*_actor);
		}
	}

	bool JoltDynamicBody::GetIsSleeping() const
	{
		if(!_isInSimulation) return true;

		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();

		return !bodyInterface.IsActive(*_actor);
	}

	void JoltDynamicBody::LockMovement(RN::uint32 lockFlags)
	{
		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		const JPH::BodyLockInterface &lockInterface = physics->GetBodyLockInterface();
		JPH::BodyLockWrite lock(lockInterface, *_actor);
		if(!lock.Succeeded()) return;

		JPH::Body &body = lock.GetBody();
		JPH::MotionProperties *mp = body.GetMotionProperties();
		if(!mp) return;

		// Build allowed DOFs: start with all DOFs enabled
		JPH::EAllowedDOFs allowed = JPH::EAllowedDOFs::All;

		// Linear locks
		if(lockFlags & LockAxis::LockAxisLinearX) allowed = static_cast<JPH::EAllowedDOFs>(uint32(allowed) & ~uint32(JPH::EAllowedDOFs::TranslationX));
		if(lockFlags & LockAxis::LockAxisLinearY) allowed = static_cast<JPH::EAllowedDOFs>(uint32(allowed) & ~uint32(JPH::EAllowedDOFs::TranslationY));
		if(lockFlags & LockAxis::LockAxisLinearZ) allowed = static_cast<JPH::EAllowedDOFs>(uint32(allowed) & ~uint32(JPH::EAllowedDOFs::TranslationZ));

		// Angular locks
		if(lockFlags & LockAxis::LockAxisAngularX) allowed = static_cast<JPH::EAllowedDOFs>(uint32(allowed) & ~uint32(JPH::EAllowedDOFs::RotationX));
		if(lockFlags & LockAxis::LockAxisAngularY) allowed = static_cast<JPH::EAllowedDOFs>(uint32(allowed) & ~uint32(JPH::EAllowedDOFs::RotationY));
		if(lockFlags & LockAxis::LockAxisAngularZ) allowed = static_cast<JPH::EAllowedDOFs>(uint32(allowed) & ~uint32(JPH::EAllowedDOFs::RotationZ));

		// Recompute mass properties for current mass
		float invMass = mp->GetInverseMassUnchecked();
		if(invMass <= 0.0f) return; // static/kinematic
		float mass = 1.0f / invMass;
		JPH::MassProperties massProps = _shape->GetJoltShape()->GetMassProperties();
		massProps.ScaleToMass(mass);
		mp->SetMassProperties(allowed, massProps);
		InvalidateMotionCache();
	}

	void JoltDynamicBody::SetSolverIterationCount(uint32 positionIterations, uint32 velocityIterations)
	{
		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		const JPH::BodyLockInterface &lockInterface = physics->GetBodyLockInterface();
		JPH::BodyLockWrite lock(lockInterface, *_actor);
		if(!lock.Succeeded()) return;

		JPH::MotionProperties *mp = lock.GetBody().GetMotionProperties();
		if(!mp) return;

		if(velocityIterations > 0) mp->SetNumVelocityStepsOverride(velocityIterations);
		if(positionIterations > 0) mp->SetNumPositionStepsOverride(positionIterations);
	}

	void JoltDynamicBody::SetEnableCCD(bool enable)
	{
		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		bodyInterface.SetMotionQuality(*_actor, enable ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete);
	}

	void JoltDynamicBody::SetEnableGravity(bool enable)
	{
		_isGravityEnabled = enable;
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;
		bodyInterface->SetGravityFactor(*_actor, enable ? 1.0f : 0.0f);
	}

	void JoltDynamicBody::SetFriction(float friction)
	{
		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		bodyInterface.SetFriction(*_actor, friction);
	}

	void JoltDynamicBody::SetRestitution(float restitution)
	{
		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		bodyInterface.SetRestitution(*_actor, restitution);
	}

	void JoltDynamicBody::AddForce(const Vector3 &force)
	{
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;

		bodyInterface->AddForce(*_actor, JoltNativeConversions::ToJoltVector(force));
	}

	void JoltDynamicBody::AddForce(const Vector3 &force, const JoltPosition &globalOrigin)
	{
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;

		bodyInterface->AddForce(*_actor, JoltNativeConversions::ToJoltVector(force), JoltNativeConversions::ToJoltPosition(globalOrigin));
	}

	void JoltDynamicBody::ApplyGravity(const Vector3 &gravity)
	{
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;
		if(_isKinematic || !_isGravityEnabled) return;
		if(_mass <= k::EpsilonFloat || gravity.GetSquaredLength() <= k::EpsilonFloat) return;

		bodyInterface->AddForce(*_actor, JoltNativeConversions::ToJoltVector(gravity * _mass), JPH::EActivation::DontActivate);
	}

	/*	void JoltDynamicBody::ClearForces()
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		
		bodyInterface.
	}*/

	void JoltDynamicBody::AddTorque(const Vector3 &torque)
	{
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;

		bodyInterface->AddTorque(*_actor, JoltNativeConversions::ToJoltVector(torque));
	}
	void JoltDynamicBody::AddTorqueImpulse(const Vector3 &torque)
	{
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;

		bodyInterface->AddAngularImpulse(*_actor, JoltNativeConversions::ToJoltVector(torque));
	}
	void JoltDynamicBody::AddImpulse(const Vector3 &impulse)
	{
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;

		bodyInterface->AddImpulse(*_actor, JoltNativeConversions::ToJoltVector(impulse));
	}
	void JoltDynamicBody::AddImpulse(const Vector3 &impulse, const JoltPosition &globalOrigin)
	{
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;

		bodyInterface->AddImpulse(*_actor, JoltNativeConversions::ToJoltVector(impulse), JoltNativeConversions::ToJoltPosition(globalOrigin));
	}

	bool JoltDynamicBody::ApplyBuoyancyImpulse(const JoltPosition &globalSurfacePosition, const Vector3 &surfaceNormal, float buoyancy, float linearDrag, float angularDrag, const Vector3 &fluidVelocity, const Vector3 &gravity, float delta)
	{
		if(delta <= k::EpsilonFloat || surfaceNormal.GetSquaredLength() <= k::EpsilonFloat) return false;

		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return false;

		Vector3 normalizedSurfaceNormal = surfaceNormal.GetNormalized();
		return bodyInterface->ApplyBuoyancyImpulse(*_actor, JoltNativeConversions::ToJoltPosition(globalSurfacePosition),
			JoltNativeConversions::ToJoltVector(normalizedSurfaceNormal),
			buoyancy,
			linearDrag,
			angularDrag,
			JoltNativeConversions::ToJoltVector(fluidVelocity),
			JoltNativeConversions::ToJoltVector(gravity),
			delta);
	}

	void JoltDynamicBody::SetEnableKinematic(bool enable)
	{
		_isKinematic = enable;
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;
		JPH::EMotionType motionType = enable ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic;
		bodyInterface->SetMotionType(*_actor, motionType, JPH::EActivation::DontActivate);
	}

	void JoltDynamicBody::SetEnableSimulation(bool enable)
	{
		if(!_actor) return;
		InvalidateMotionCache();

		JoltWorld *world = GetJoltWorld();
		JPH::PhysicsSystem *physics = world->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		bool isInSimulation = bodyInterface.IsAdded(*_actor);
		_isInSimulation = isInSimulation;
		if(isInSimulation == enable)
		{
			if(enable)
			{
				world->CancelQueuedBodyRemoval(*_actor);
				bodyInterface.SetObjectLayer(*_actor, world->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1));
				bodyInterface.SetGravityFactor(*_actor, _isGravityEnabled ? 1.0f : 0.0f);
				bodyInterface.SetMotionType(*_actor, _isKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic, JPH::EActivation::DontActivate);
			}
			return;
		}

		if(enable)
		{
			world->CancelQueuedBodyRemoval(*_actor);

			RN::Quaternion worldRotation = GetWorldRotation();
			RN::PositionType worldPosition = GetWorldPosition();
			if(worldPosition.IsValid() && worldRotation.IsValid())
			{
				worldRotation.Normalize();
				RN::Vector3 positionOffset = worldRotation.GetRotatedVector(_positionOffset);
				JPH::RVec3 position = JoltNativeConversions::GetAttachmentPosition(this, -positionOffset, GetJoltWorld());
				RN::Quaternion rotation = worldRotation * _rotationOffset;
				rotation.Normalize();
				bodyInterface.SetPositionAndRotation(*_actor, position, JoltNativeConversions::ToJoltSceneRotation(rotation, GetJoltWorld()), JPH::EActivation::DontActivate);
			}
			bodyInterface.AddBody(*_actor, JPH::EActivation::Activate);
			_isInSimulation = true;
			bodyInterface.SetObjectLayer(*_actor, world->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1));
			bodyInterface.SetGravityFactor(*_actor, _isGravityEnabled ? 1.0f : 0.0f);
			bodyInterface.SetMotionType(*_actor, _isKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic, JPH::EActivation::DontActivate);
			return;
		}

		if(isInSimulation)
		{
			bodyInterface.SetLinearAndAngularVelocity(*_actor, JPH::Vec3::sZero(), JPH::Vec3::sZero());
			bodyInterface.SetGravityFactor(*_actor, 0.0f);
			bodyInterface.SetObjectLayer(*_actor, world->GetObjectLayer(_collisionFilterGroup, 0, 1));
			bodyInterface.SetMotionType(*_actor, JPH::EMotionType::Kinematic, JPH::EActivation::DontActivate);
			bodyInterface.DeactivateBody(*_actor);
			world->QueueBodyRemoval(*_actor);
		}
		_isInSimulation = false;
	}

	bool JoltDynamicBody::GetIsKinematic() const
	{
		return _isKinematic;
	}

	void JoltDynamicBody::SetKinematicTarget(const JoltPosition &globalPosition, const Quaternion &rotation, float delta)
	{
		if(!globalPosition.IsValid() || !rotation.IsValid())
			return;

		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface) return;

		Quaternion targetWorldRotation = rotation;
		if(!targetWorldRotation.IsValid())
			return;
		targetWorldRotation.Normalize();

		Vector3 positionOffset = targetWorldRotation.GetRotatedVector(_positionOffset);
		JoltPosition targetPositionVector = globalPosition - JoltPosition(positionOffset);
		if(!targetPositionVector.IsValid())
			return;

		Quaternion targetRotation = targetWorldRotation * _rotationOffset;
		if(!targetRotation.IsValid())
			return;

		targetRotation.Normalize();

		JPH::RVec3 targetPosition = JoltNativeConversions::ToJoltPosition(targetPositionVector);
		JPH::Quat targetJoltRotation = JoltNativeConversions::ToJoltRotation(targetRotation);
		targetJoltRotation = targetJoltRotation.Normalized();
		if(targetJoltRotation.IsNaN() || !targetJoltRotation.IsNormalized())
			return;

		auto setTargetPoseDirectly = [&]() {
			bodyInterface->SetPositionAndRotation(*_actor, targetPosition, targetJoltRotation, JPH::EActivation::DontActivate);
			bodyInterface->SetLinearAndAngularVelocity(*_actor, JPH::Vec3::sZero(), JPH::Vec3::sZero());
		};

		if(!std::isfinite(delta) || delta <= k::EpsilonFloat)
		{
			setTargetPoseDirectly();
		}
		else
		{
			bodyInterface->MoveKinematic(*_actor, targetPosition, targetJoltRotation, delta);
		}
	}

	/*	void JoltDynamicBody::AccelerateToTarget(const Vector3 &position, const Quaternion &rotation, float delta)
	{
		//Linear velocity
		RN::Vector3 speed = position - GetWorldPosition();
		speed /= delta;

		//Angular velocity
		RN::Quaternion startRotation = GetWorldRotation();
		if(rotation.GetDotProduct(startRotation) > 0.0f)
			startRotation = startRotation.GetConjugated();
		RN::Quaternion rotationSpeed = rotation*startRotation;
		RN::Vector4 axisAngleSpeed = rotationSpeed.GetAxisAngle();
		if(axisAngleSpeed.w > 180.0f)
			axisAngleSpeed.w -= 360.0f;
		RN::Vector3 angularVelocity(axisAngleSpeed.x, axisAngleSpeed.y, axisAngleSpeed.z);
		angularVelocity *= axisAngleSpeed.w*M_PI;
		angularVelocity /= 180.0f;
		angularVelocity /= delta;

		RN::Vector3 linearForce = speed - GetLinearVelocity();
		linearForce /= delta;
//		linearForce *= _actor->getMass();
		linearForce *= 10.0f;
		RN::Vector3 angularForce = angularVelocity - GetAngularVelocity();
		angularForce /= delta;
//		angularForce *= _mass;
		angularForce *= 10.0f;

		if(linearForce.GetLength() > 5000.0f)
			linearForce.Normalize(5000.0f);

		if(angularForce.GetLength() > 15000.0f)
			angularForce.Normalize(15000.0f);
		
		_actor->addForce(Jolt::PxVec3(linearForce.x, linearForce.y, linearForce.z), Jolt::PxForceMode::eACCELERATION);
		_actor->addTorque(Jolt::PxVec3(angularForce.x, angularForce.y, angularForce.z), Jolt::PxForceMode::eACCELERATION);
	}
	
	bool JoltDynamicBody::SweepTest(std::vector<JoltContactInfo> &contactInfo, const Vector3 &direction, const Vector3 &offsetPosition, const Quaternion &offsetRotation, float inflation) const
	{
		Jolt::PxTransform pose = _actor->getGlobalPose();
		pose.p += Jolt::PxVec3(offsetPosition.x, offsetPosition.y, offsetPosition.z);
		pose.q = Jolt::PxQuat(offsetRotation.x, offsetRotation.y, offsetRotation.z, offsetRotation.w) * pose.q;

		Jolt::PxScene *scene = JoltWorld::GetSharedInstance()->GetJoltScene();
		float length = direction.GetLength();
		Jolt::PxVec3 normalizedDirection = Jolt::PxVec3(direction.x, direction.y, direction.z);
		if(normalizedDirection.magnitude() < k::EpsilonFloat)
			normalizedDirection = Jolt::PxVec3(0.0f, 0.0f, -1.0f);
		normalizedDirection.normalize();
		Jolt::PxSweepBuffer hit;
		Jolt::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;

		JoltQueryFilterCallback filterCallback;

		if(_shape->IsKindOfClass(JoltCompoundShape::GetMetaClass()))
		{
			JoltCompoundShape *compound = _shape->Downcast<JoltCompoundShape>();
			for(JoltShape *tempShape : compound->_shapes)
			{
				Jolt::PxShape *shape = tempShape->GetJoltShape();
				shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
			}
			for(JoltShape *tempShape : compound->_shapes)
			{
				Jolt::PxShape *shape = tempShape->GetJoltShape();
				scene->sweep(shape->getGeometry().any(), pose, normalizedDirection, length, hit, Jolt::PxHitFlags(Jolt::PxHitFlag::eDEFAULT), Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eDYNAMIC | Jolt::PxQueryFlag::eSTATIC | Jolt::PxQueryFlag::ePREFILTER|Jolt::PxQueryFlag::eNO_BLOCK), &filterCallback, nullptr, inflation);

				for(int i = 0; i < hit.getNbAnyHits(); i++)
				{
					JoltContactInfo contact;
					contact.distance = hit.getAnyHit(i).distance;
					contact.normal = Vector3(hit.getAnyHit(i).normal.x, hit.getAnyHit(i).normal.y, hit.getAnyHit(i).normal.z);
					contact.position = Vector3(hit.getAnyHit(i).position.x, hit.getAnyHit(i).position.y, hit.getAnyHit(i).position.z);
					contact.node = nullptr;
					JoltCollisionObject *attachment = static_cast<JoltCollisionObject*>(hit.getAnyHit(i).actor->userData);
					contact.collisionObject = attachment;
					if(attachment)
					{
						contact.node = attachment->GetParent();
						if(contact.node) contact.node->Retain()->Autorelease();
					}
					contactInfo.push_back(contact);
				}
			}
			for(JoltShape *tempShape : compound->_shapes)
			{
				Jolt::PxShape *shape = tempShape->GetJoltShape();
				shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, true);
			}
		}
		else if(_shape->GetJoltShape())
		{
			Jolt::PxShape *shape = _shape->GetJoltShape();
			shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
			scene->sweep(shape->getGeometry().any(), pose, normalizedDirection, length, hit, Jolt::PxHitFlags(Jolt::PxHitFlag::eDEFAULT), Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eDYNAMIC | Jolt::PxQueryFlag::eSTATIC | Jolt::PxQueryFlag::ePREFILTER|Jolt::PxQueryFlag::eNO_BLOCK), &filterCallback, nullptr, inflation);
			shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, true);

			for(int i = 0; i < hit.getNbAnyHits(); i++)
			{
				JoltContactInfo contact;
				contact.distance = hit.getAnyHit(i).distance;
				contact.normal = Vector3(hit.getAnyHit(i).normal.x, hit.getAnyHit(i).normal.y, hit.getAnyHit(i).normal.z);
				contact.position = Vector3(hit.getAnyHit(i).position.x, hit.getAnyHit(i).position.y, hit.getAnyHit(i).position.z);
				contact.node = nullptr;
				JoltCollisionObject *attachment = static_cast<JoltCollisionObject*>(hit.getAnyHit(i).actor->userData);
				contact.collisionObject = attachment;
				if(attachment)
				{
					contact.node = attachment->GetParent();
					if(contact.node) contact.node->Retain()->Autorelease();
				}
				contactInfo.push_back(contact);
			}
		}

		return (contactInfo.size() > 0);
	}

	Quaternion JoltDynamicBody::RotationSweepTest(std::vector<JoltContactInfo> &contactInfo, const Quaternion &targetRoation, float stepSize, float sweepSize, const Vector3 &offsetPosition, const Quaternion &offsetRotation) const
	{
		Quaternion startRotation = offsetRotation*GetWorldRotation();
		Quaternion rotationDiff = targetRoation / startRotation;
		rotationDiff.Normalize();
		Vector4 axisAngleDiff = rotationDiff.GetAxisAngle();
		if(axisAngleDiff.w < stepSize)
			return GetWorldRotation();

		uint32 maxSteps = axisAngleDiff.w / stepSize + 1;
		float actualStepSize = 1.0f / static_cast<float>(maxSteps);
		float slerpFactor = actualStepSize;

		Quaternion lastRotation = startRotation;
		Quaternion newRotation = startRotation.GetLerpSpherical(targetRoation, slerpFactor).GetNormalized();
		Vector3 lastDirection = lastRotation.GetRotatedVector(Vector3(0.0f, 0.0f, -1.0f));
		Vector3 newDirection = newRotation.GetRotatedVector(Vector3(0.0f, 0.0f, -1.0f));
		Vector3 directionDiff = newDirection - lastDirection;
		directionDiff.Normalize(sweepSize);

		while(slerpFactor < 1.0f)
		{
			bool isBlocked = SweepTest(contactInfo, directionDiff * 2.0f, offsetPosition - directionDiff, newRotation / GetWorldRotation());
			if(isBlocked)
			{
				return lastRotation;
			}

			slerpFactor += actualStepSize;
			lastRotation = newRotation;
			newRotation = startRotation.GetLerpSpherical(targetRoation, slerpFactor).GetNormalized();

			lastDirection = lastRotation.GetRotatedVector(Vector3(0.0f, 0.0f, -1.0f));
			newDirection = newRotation.GetRotatedVector(Vector3(0.0f, 0.0f, -1.0f));
			directionDiff = newDirection - lastDirection;
			directionDiff.Normalize(sweepSize);
		}

		return newRotation;
	}*/

	void JoltDynamicBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		JoltCollisionObject::DidUpdate(changeSet);
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!bodyInterface)
		{
			if(changeSet & SceneNode::ChangeSet::Attachments)
			{
				_owner = GetParent();
			}
			return;
		}

		bool shouldUpdatePose = (changeSet & SceneNode::ChangeSet::Position) || ((changeSet & SceneNode::ChangeSet::Attachments) && !_owner && GetParent());
		if(shouldUpdatePose)
		{
			RN::Quaternion worldRotation = GetWorldRotation();
			if(worldRotation.IsValid())
			{
				worldRotation.Normalize();

				RN::Vector3 positionOffset = worldRotation.GetRotatedVector(_positionOffset);
				JPH::RVec3 position = JoltNativeConversions::GetAttachmentPosition(this, -positionOffset, GetJoltWorld());
				Quaternion rotation = worldRotation * _rotationOffset;
				if(rotation.IsValid())
				{
					rotation.Normalize();
					bodyInterface->SetPositionAndRotation(*_actor, position, JoltNativeConversions::ToJoltSceneRotation(rotation, GetJoltWorld()), JPH::EActivation::DontActivate);
				}
			}
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			_owner = GetParent();
		}
	}

	/*	void JoltDynamicBody::UpdateFromMaterial(BulletMaterial *material)
	{
		_rigidBody->setFriction(material->GetFriction());
		_rigidBody->setRollingFriction(material->GetRollingFriction());
		_rigidBody->setSpinningFriction(material->GetSpinningFriction());
		_rigidBody->setRestitution(material->GetRestitution());
		_rigidBody->setDamping(material->GetLinearDamping(), material->GetAngularDamping());
	}*/

	void JoltDynamicBody::UpdatePosition()
	{
		JPH::BodyInterface *bodyInterface = GetBodyInterfaceIfInSimulation();
		if(!GetParent() || !bodyInterface)
		{
			return;
		}

		JPH::RVec3 position;
		JPH::Quat rotation;
		bodyInterface->GetPositionAndRotation(*_actor, position, rotation);

		RN::Quaternion rotationResult = JoltNativeConversions::ToSceneRotation(rotation, GetJoltWorld()) * _rotationOffset.GetConjugated();
		RN::Vector3 positionOffset = rotationResult.GetRotatedVector(_positionOffset);
		JoltNativeConversions::SetAttachmentTransform(this, position, positionOffset, rotationResult, GetJoltWorld());
	}
} // namespace RN
