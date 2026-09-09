//
//  RNJoltRigidBodyController.cpp
//  Rayne-Jolt
//
//  Copyright 2026 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltRigidBodyController.h"
#include "RNJoltExternalSupportAnchorConstraint.h"
#include "RNJoltInternals.h"
#include "RNJoltWorld.h"

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/MotionProperties.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Constraints/Constraint.h>

namespace RN
{
	RNDefineMeta(JoltRigidBodyController, JoltCollisionObject)

	constexpr uint32 InvalidSupportBodyID = 0xffffffff;
	constexpr float ExternalSupportAnchorCorrectionSlop = 0.05f;
	constexpr float ExternalSupportAnchorCorrectionFrequency = 8.0f;
	constexpr float ExternalSupportAnchorMaxCorrectionSpeed = 20.0f;

	JoltRigidBodyController::JoltRigidBodyController(float radius, float height, float groundTolerance, float mass, float stepOffset) :
		_radius(radius), _height(height), _groundTolerance(groundTolerance), _mass(mass), _stepOffset(stepOffset), _upDirection(0.0f, 1.0f, 0.0f), _objectBelow(nullptr), _groundVelocity(), _groundAngularVelocity(), _groundNormal(), _isFalling(false), _externalSupportAnchorValid(false), _externalSupportCollisionFilteringEnabled(false), _externalSupportBodyID(InvalidSupportBodyID), _externalSupportLocalPosition(), _externalSupportAnchorMaxForce(0.0f), _externalSupportConstraint(nullptr)
	{
		JoltWorld *world = JoltWorld::GetSharedInstance();
		JPH::PhysicsSystem *physics = world->GetJoltInstance();

		_shape = JoltCapsuleShape::WithRadius(radius, height)->Retain();

		JPH::CharacterSettings settings;
		settings.mUp = JoltConversions::ToJoltVector(_upDirection);
		settings.mMaxSlopeAngle = JPH::DegreesToRadians(70.0f);
		settings.mMass = _mass;
		// Movement and support velocity are supplied explicitly by Move(). Contact
		// friction would otherwise erase sufficiently small movement commands.
		settings.mFriction = 0.0f;
		settings.mGravityFactor = 0.0f;
		settings.mLayer = world->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1);
		settings.mShape = _shape->GetJoltShape();
		settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -radius);

		_controller = new JPH::Character(&settings, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), reinterpret_cast<uint64>(this), physics);
		{
			JPH::BodyLockWrite lock(physics->GetBodyLockInterface(), _controller->GetBodyID());
			if(lock.Succeeded())
			{
				JPH::MotionProperties *motionProperties = lock.GetBody().GetMotionProperties();
				if(motionProperties) motionProperties->SetLinearDamping(0.0f);
			}
		}
		_controller->AddToPhysicsSystem(JPH::EActivation::DontActivate);
		SetMaxLinearVelocity(world->GetDefaultDynamicBodyMaxLinearVelocity());
	}

	JoltRigidBodyController::~JoltRigidBodyController()
	{
		ClearExternalSupportAnchor();
		_controller->RemoveFromPhysicsSystem();
		delete _controller;
		SafeRelease(_shape);
	}

	uint32 JoltRigidBodyController::GetJoltBodyID() const
	{
		return _controller->GetBodyID().GetIndexAndSequenceNumber();
	}

	void JoltRigidBodyController::Move(const Vector3 &velocity, float delta)
	{
		if(delta <= k::EpsilonFloat)
		{
			UpdateGroundState();
			return;
		}

		Vector3 adjustedVelocity = GetGroundAdjustedVelocity(velocity);
		if(_externalSupportAnchorValid)
		{
			JoltPosition supportAnchorPosition;
			Vector3 supportAnchorVelocity;
			if(GetExternalSupportAnchorState(supportAnchorPosition, supportAnchorVelocity))
			{
				adjustedVelocity += supportAnchorVelocity;

				Vector3 anchorDelta(supportAnchorPosition - JoltConversions::ToPosition(_controller->GetPosition()));
				float anchorDistance = anchorDelta.GetLength();
				if(anchorDistance > k::EpsilonFloat)
				{
					anchorDelta.Normalize();
					float correctionDistance = anchorDistance - ExternalSupportAnchorCorrectionSlop;
					if(correctionDistance > 0.0f)
					{
						float correctingSpeed = correctionDistance * ExternalSupportAnchorCorrectionFrequency;
						if(correctingSpeed > ExternalSupportAnchorMaxCorrectionSpeed)
						{
							correctingSpeed = ExternalSupportAnchorMaxCorrectionSpeed;
						}

						adjustedVelocity += anchorDelta * correctingSpeed;
					}
				}
			}
		}

		ApplyStepOffset(adjustedVelocity, delta);
		SetLinearVelocity(adjustedVelocity);
	}

	void JoltRigidBodyController::SetLinearVelocity(const Vector3 &velocity)
	{
		if(!velocity.IsValid())
		{
			return;
		}

		_controller->SetLinearVelocity(JoltConversions::ToJoltVector(velocity));
		_controller->Activate();
	}

	void JoltRigidBodyController::SetMaxLinearVelocity(float max)
	{
		if(max <= k::EpsilonFloat)
		{
			return;
		}

		JPH::BodyInterface &bodyInterface = JoltWorld::GetSharedInstance()->GetJoltInstance()->GetBodyInterface();
		JPH::BodyID bodyID = _controller->GetBodyID();
		if(bodyID.IsInvalid())
		{
			return;
		}

		bodyInterface.SetMaxLinearVelocity(bodyID, max);
	}

	void JoltRigidBodyController::ApplyGravity(const Vector3 &gravity)
	{
		if(!gravity.IsValid() || gravity.GetSquaredLength() <= k::EpsilonFloat || _mass <= k::EpsilonFloat)
		{
			return;
		}

		Vector3 appliedGravity = gravity;
		if(_controller->GetGroundState() == JPH::Character::EGroundState::OnGround && _groundNormal.GetSquaredLength() > k::EpsilonFloat)
		{
			Vector3 groundNormal = _groundNormal.GetNormalized();
			float normalGravity = gravity.GetDotProduct(groundNormal);
			Vector3 relativeVelocity = GetLinearVelocity() - _groundVelocity;
			if(normalGravity < 0.0f && relativeVelocity.GetDotProduct(groundNormal) <= k::EpsilonFloat)
			{
				// The controller follows the ground explicitly. Keep only the force that
				// holds it against a walkable surface so a frictionless motor cannot drift downhill.
				appliedGravity = groundNormal * normalGravity;
			}
		}

		JPH::BodyInterface &bodyInterface = JoltWorld::GetSharedInstance()->GetJoltInstance()->GetBodyInterface();
		bodyInterface.AddForce(_controller->GetBodyID(), JoltConversions::ToJoltVector(appliedGravity * _mass), JPH::EActivation::DontActivate);
	}

	Vector3 JoltRigidBodyController::GetLinearVelocity() const
	{
		return JoltConversions::ToEngineVector(_controller->GetLinearVelocity());
	}

	bool JoltRigidBodyController::Resize(float height, bool checkIfBlocked)
	{
		float heightDifference = height - _height;
		if(((heightDifference < 0.0f) ? -heightDifference : heightDifference) < k::EpsilonFloat) return true;

		JoltShape *newShape = JoltCapsuleShape::WithRadius(_radius, height);
		bool didSetShape = _controller->SetShape(newShape->GetJoltShape(), checkIfBlocked ? 0.01f : 1000000.0f);
		if(!didSetShape) return false;

		SafeRelease(_shape);
		_shape = newShape->Retain();
		_height = height;
		return true;
	}

	void JoltRigidBodyController::SetCollisionFilter(uint32 group, uint32 mask)
	{
		JoltCollisionObject::SetCollisionFilter(group, mask);

		JoltWorld *world = JoltWorld::GetSharedInstance();
		_controller->SetLayer(world->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1));
	}

	void JoltRigidBodyController::SetUpDirection(const Vector3 &upDirection)
	{
		if(!upDirection.IsValid() || upDirection.GetSquaredLength() <= k::EpsilonFloat)
		{
			return;
		}

		_upDirection = upDirection.GetNormalized();
		JPH::Vec3 joltUpDirection = JoltConversions::ToJoltVector(_upDirection);
		_controller->SetUp(joltUpDirection);
		_controller->SetSupportingVolume(JPH::Plane((_controller->GetRotation().Conjugated() * joltUpDirection).NormalizedOr(JPH::Vec3::sAxisY()), -_radius));
		_controller->Activate();
	}

	Vector3 JoltRigidBodyController::GetFeetOffset() const
	{
		return Vector3(0.0f, -(_height * 0.5f + _radius), 0.0f);
	}

	void JoltRigidBodyController::SetExternalSupportAnchor(uint32 bodyID, const Vector3 &localPosition, float maxForce)
	{
		if(bodyID == InvalidSupportBodyID || !localPosition.IsValid() || maxForce <= k::EpsilonFloat)
		{
			ClearExternalSupportAnchor();
			return;
		}

		if(_externalSupportAnchorValid && _externalSupportConstraint && _externalSupportBodyID == bodyID && _externalSupportLocalPosition == localPosition && _externalSupportAnchorMaxForce == maxForce)
		{
			if(IsExternalSupportBodyUsable(bodyID))
			{
				return;
			}

			ClearExternalSupportAnchor();
			return;
		}

		ClearExternalSupportAnchor();
		if(!IsExternalSupportBodyUsable(bodyID))
		{
			return;
		}

		JoltPosition supportPosition;
		Quaternion supportRotation;
		if(!GetSupportBodyTransform(bodyID, supportPosition, supportRotation))
		{
			return;
		}

		JoltPosition supportAnchorPosition = supportPosition + supportRotation.GetRotatedVector(localPosition);
		JoltPosition controllerAnchorPosition = JoltConversions::ToPosition(JoltConversions::GetAttachmentPosition(this));
		if(!supportAnchorPosition.IsValid() || !controllerAnchorPosition.IsValid())
		{
			return;
		}

		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		JPH::Constraint *constraint = CreateJoltExternalSupportAnchorConstraint(bodyInterface, JPH::BodyID(bodyID), _controller->GetBodyID(), JoltConversions::ToJoltPosition(supportAnchorPosition), JoltConversions::ToJoltPosition(controllerAnchorPosition), maxForce);
		if(!constraint)
		{
			return;
		}

		constraint->SetNumVelocityStepsOverride(10);
		constraint->SetNumPositionStepsOverride(10);
		constraint->SetUserData(reinterpret_cast<uint64>(static_cast<JoltConstraintOwner *>(this)));
		physics->AddConstraint(constraint);
		JoltWorld::GetSharedInstance()->SetConnectedBodyCollisionFilteringEnabled(JPH::BodyID(bodyID), _controller->GetBodyID(), true);

		_externalSupportAnchorValid = true;
		_externalSupportCollisionFilteringEnabled = true;
		_externalSupportBodyID = bodyID;
		_externalSupportLocalPosition = localPosition;
		_externalSupportAnchorMaxForce = maxForce;
		_externalSupportConstraint = constraint;

		bodyInterface.ActivateBody(JPH::BodyID(bodyID));
		_controller->Activate();
	}

	void JoltRigidBodyController::ClearExternalSupportAnchor()
	{
		if(JoltWorld *world = JoltWorld::GetSharedInstance())
		{
			if(_externalSupportCollisionFilteringEnabled)
			{
				world->SetConnectedBodyCollisionFilteringEnabled(JPH::BodyID(_externalSupportBodyID), _controller->GetBodyID(), false);
			}

			if(_externalSupportConstraint)
			{
				_externalSupportConstraint->SetEnabled(false);
				world->GetJoltInstance()->RemoveConstraint(_externalSupportConstraint);
			}
		}

		_externalSupportAnchorValid = false;
		_externalSupportCollisionFilteringEnabled = false;
		_externalSupportBodyID = InvalidSupportBodyID;
		_externalSupportLocalPosition = Vector3();
		_externalSupportAnchorMaxForce = 0.0f;
		_externalSupportConstraint = nullptr;
	}

	void JoltRigidBodyController::InvalidateJoltConstraint(JPH::Constraint *constraint)
	{
		if(_externalSupportConstraint != constraint) return;

		_externalSupportConstraint = nullptr;
		ClearExternalSupportAnchor();
	}

	Vector3 JoltRigidBodyController::GetGroundAdjustedVelocity(const Vector3 &velocity) const
	{
		if(_controller->GetGroundState() != JPH::Character::EGroundState::OnGround)
		{
			return velocity;
		}

		Vector3 relativeVelocity = velocity - _groundVelocity;
		Vector3 horizontalVelocity = relativeVelocity - _upDirection * relativeVelocity.GetDotProduct(_upDirection);
		float horizontalSpeed = horizontalVelocity.GetLength();

		if(horizontalSpeed <= k::EpsilonFloat)
		{
			return velocity;
		}

		Vector3 groundNormal = _groundNormal;
		if(groundNormal.GetSquaredLength() <= k::EpsilonFloat || groundNormal.GetDotProduct(_upDirection) <= k::EpsilonFloat)
		{
			return velocity;
		}

		groundNormal.Normalize();
		Vector3 slopeVelocity = horizontalVelocity - groundNormal * horizontalVelocity.GetDotProduct(groundNormal);
		if(slopeVelocity.GetSquaredLength() <= k::EpsilonFloat)
		{
			return velocity;
		}

		slopeVelocity.Normalize(horizontalSpeed);
		float upwardSpeed = relativeVelocity.GetDotProduct(_upDirection);
		if(upwardSpeed > 0.0f)
		{
			slopeVelocity += _upDirection * upwardSpeed;
		}

		return _groundVelocity + slopeVelocity;
	}

	void JoltRigidBodyController::ApplyStepOffset(const Vector3 &velocity, float delta)
	{
		if(_stepOffset <= k::EpsilonFloat || !_controller->IsSupported())
		{
			return;
		}

		Vector3 relativeVelocity = velocity - _groundVelocity;
		Vector3 horizontalVelocity = relativeVelocity - _upDirection * relativeVelocity.GetDotProduct(_upDirection);
		if(horizontalVelocity.GetSquaredLength() <= k::EpsilonFloat)
		{
			return;
		}

		RN::Vector3 movementDirection = horizontalVelocity;
		movementDirection.Normalize();

		JoltPosition currentPosition = JoltConversions::ToPosition(_controller->GetPosition());
		JPH::Quat rotation = _controller->GetRotation();
		Quaternion currentRotation = JoltConversions::ToEngineRotation(rotation);
		Vector3 horizontalMovement = horizontalVelocity * delta;
		float horizontalDistance = horizontalMovement.GetLength();
		if(horizontalDistance <= k::EpsilonFloat)
		{
			return;
		}

		Vector3 stepProbeMovement = relativeVelocity * delta;
		if(horizontalDistance < 0.02f)
		{
			stepProbeMovement *= 0.02f / horizontalDistance;
		}

		if(!HasBlockingCollisionAt(currentPosition + stepProbeMovement, currentRotation, movementDirection))
		{
			return;
		}

		constexpr int StepSearchCount = 8;
		constexpr int StepRefinementCount = 5;
		constexpr float StepClearance = 0.005f;

		float blockedHeight = 0.0f;
		float clearHeight = -1.0f;
		for(int i = 1; i <= StepSearchCount; i++)
		{
			float candidateHeight = _stepOffset * (static_cast<float>(i) / static_cast<float>(StepSearchCount));
			JoltPosition candidateStep(_upDirection * candidateHeight);
			if(!HasPenetrationAt(currentPosition + candidateStep, currentRotation, movementDirection) && !HasPenetrationAt(currentPosition + candidateStep + stepProbeMovement, currentRotation, movementDirection))
			{
				clearHeight = candidateHeight;
				break;
			}

			blockedHeight = candidateHeight;
		}

		if(clearHeight < 0.0f)
		{
			return;
		}

		for(int i = 0; i < StepRefinementCount; i++)
		{
			float candidateHeight = (blockedHeight + clearHeight) * 0.5f;
			JoltPosition candidateStep(_upDirection * candidateHeight);
			if(!HasPenetrationAt(currentPosition + candidateStep, currentRotation, movementDirection) && !HasPenetrationAt(currentPosition + candidateStep + stepProbeMovement, currentRotation, movementDirection))
			{
				clearHeight = candidateHeight;
			}
			else
			{
				blockedHeight = candidateHeight;
			}
		}

		float stepHeight = clearHeight + StepClearance;
		if(stepHeight > _stepOffset) stepHeight = _stepOffset;

		JoltPosition steppedPosition = currentPosition + _upDirection * stepHeight;
		_controller->SetPosition(JoltConversions::ToJoltPosition(steppedPosition), JPH::EActivation::Activate);
	}

	bool JoltRigidBodyController::HasBlockingCollisionAt(const JoltPosition &position, const Quaternion &rotation, const Vector3 &movementDirection) const
	{
		JPH::RVec3 joltPosition = JoltConversions::ToJoltPosition(position);
		JPH::Quat joltRotation = JoltConversions::ToJoltRotation(rotation);
		JPH::Vec3 joltMovementDirection = JoltConversions::ToJoltVector(movementDirection);
		JPH::Vec3 joltUpDirection = JoltConversions::ToJoltVector(_upDirection);
		JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> hits;
		_controller->CheckCollision(joltPosition, joltRotation, joltMovementDirection, 0.0f, _shape->GetJoltShape(), joltPosition, hits);

		for(const JPH::CollideShapeResult &hit : hits.mHits)
		{
			if(hit.mPenetrationDepth <= k::EpsilonFloat)
			{
				continue;
			}

			JPH::Vec3 normal = -hit.mPenetrationAxis.NormalizedOr(JPH::Vec3::sZero());
			if(normal.Dot(joltUpDirection) < 0.5f)
			{
				return true;
			}
		}

		return false;
	}

	bool JoltRigidBodyController::HasPenetrationAt(const JoltPosition &position, const Quaternion &rotation, const Vector3 &movementDirection) const
	{
		JPH::RVec3 joltPosition = JoltConversions::ToJoltPosition(position);
		JPH::Quat joltRotation = JoltConversions::ToJoltRotation(rotation);
		JPH::Vec3 joltMovementDirection = JoltConversions::ToJoltVector(movementDirection);
		JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> hits;
		_controller->CheckCollision(joltPosition, joltRotation, joltMovementDirection, 0.0f, _shape->GetJoltShape(), joltPosition, hits);

		for(const JPH::CollideShapeResult &hit : hits.mHits)
		{
			if(hit.mPenetrationDepth > k::EpsilonFloat)
			{
				return true;
			}
		}

		return false;
	}

	bool JoltRigidBodyController::GetSupportBodyTransform(uint32 bodyID, JoltPosition &position, Quaternion &rotation) const
	{
		JPH::BodyID joltBodyID(bodyID);
		if(joltBodyID.IsInvalid())
		{
			return false;
		}

		JPH::BodyInterface &bodyInterface = JoltWorld::GetSharedInstance()->GetJoltInstance()->GetBodyInterface();

		JPH::RVec3 joltPosition;
		JPH::Quat joltRotation;
		bodyInterface.GetPositionAndRotation(joltBodyID, joltPosition, joltRotation);
		position = JoltConversions::ToPosition(joltPosition);
		rotation = JoltConversions::ToEngineRotation(joltRotation);
		if(!position.IsValid() || !rotation.IsValid())
		{
			return false;
		}

		rotation.Normalize();
		return true;
	}

	bool JoltRigidBodyController::IsExternalSupportBodyUsable(uint32 bodyID) const
	{
		if(bodyID == InvalidSupportBodyID) return false;

		JPH::BodyID joltBodyID(bodyID);
		if(joltBodyID.IsInvalid()) return false;

		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		if(!bodyInterface.IsAdded(joltBodyID)) return false;
		return true;
	}

	bool JoltRigidBodyController::GetExternalSupportAnchorState(JoltPosition &position, Vector3 &velocity) const
	{
		if(!_externalSupportAnchorValid || _externalSupportBodyID == InvalidSupportBodyID)
		{
			return false;
		}

		JPH::BodyID joltBodyID(_externalSupportBodyID);
		if(joltBodyID.IsInvalid())
		{
			return false;
		}

		JPH::BodyInterface &bodyInterface = JoltWorld::GetSharedInstance()->GetJoltInstance()->GetBodyInterface();
		if(!bodyInterface.IsAdded(joltBodyID))
		{
			return false;
		}

		JPH::RVec3 joltPosition;
		JPH::Quat joltRotation;
		bodyInterface.GetPositionAndRotation(joltBodyID, joltPosition, joltRotation);
		JoltPosition supportPosition = JoltConversions::ToPosition(joltPosition);
		Quaternion supportRotation = JoltConversions::ToEngineRotation(joltRotation);
		if(!supportPosition.IsValid() || !supportRotation.IsValid())
		{
			return false;
		}

		supportRotation.Normalize();
		JoltPosition anchorPosition = supportPosition + supportRotation.GetRotatedVector(_externalSupportLocalPosition);
		if(!anchorPosition.IsValid())
		{
			return false;
		}

		JPH::Vec3 joltVelocity = bodyInterface.GetPointVelocity(joltBodyID, JoltConversions::ToJoltPosition(anchorPosition));
		position = anchorPosition;
		velocity = JoltConversions::ToEngineVector(joltVelocity);
		return velocity.IsValid();
	}

	void JoltRigidBodyController::UpdateControllerTransform()
	{
		Quaternion worldRotation = GetWorldRotation();
		if(!worldRotation.IsValid()) return;
		worldRotation.Normalize();

		Vector3 positionOffset = worldRotation.GetRotatedVector(_positionOffset);
		JPH::RVec3 position = JoltConversions::GetAttachmentPosition(this, -positionOffset);
		Quaternion rotation = worldRotation * _rotationOffset;
		if(!rotation.IsValid()) return;
		rotation.Normalize();

		_controller->SetPositionAndRotation(position, JoltConversions::ToJoltSceneRotation(rotation), JPH::EActivation::DontActivate);
	}

	void JoltRigidBodyController::UpdateGroundState()
	{
		if(!_controller->IsSupported())
		{
			_objectBelow = nullptr;
			_groundVelocity = Vector3();
			_groundAngularVelocity = Vector3();
			_groundNormal = Vector3();
			_isFalling = true;
			return;
		}

		JoltCollisionObject *collisionObject = reinterpret_cast<JoltCollisionObject *>(_controller->GetGroundUserData());
		_objectBelow = collisionObject ? collisionObject->GetParent() : nullptr;
		if(_objectBelow) _objectBelow->Retain()->Autorelease();

		JPH::Vec3 groundVelocity = _controller->GetGroundVelocity();
		_groundVelocity = JoltConversions::ToEngineVector(groundVelocity);
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::Vec3 groundAngularVelocity = physics->GetBodyInterface().GetAngularVelocity(_controller->GetGroundBodyID());
		_groundAngularVelocity = JoltConversions::ToEngineVector(groundAngularVelocity);
		JPH::Vec3 groundNormal = _controller->GetGroundNormal();
		_groundNormal = JoltConversions::ToEngineVector(groundNormal);
		_isFalling = false;
	}

	void JoltRigidBodyController::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		JoltCollisionObject::DidUpdate(changeSet);

		if(changeSet & SceneNode::ChangeSet::Position)
		{
			UpdateControllerTransform();
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent())
			{
				UpdateControllerTransform();
			}

			_owner = GetParent();
		}
	}

	void JoltRigidBodyController::UpdatePosition()
	{
		if(!_owner)
		{
			return;
		}

		JPH::RVec3 position;
		JPH::Quat rotation;
		_controller->PostSimulation(_groundTolerance);
		_controller->GetPositionAndRotation(position, rotation);
		UpdateGroundState();

		Quaternion rotationResult = JoltConversions::ToSceneRotation(rotation) * _rotationOffset.GetConjugated();
		Vector3 positionOffset = rotationResult.GetRotatedVector(_positionOffset);
		JoltConversions::SetAttachmentTransform(this, position, positionOffset, rotationResult);
	}
} // namespace RN
