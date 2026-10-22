//
//  RNJoltKinematicController.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltKinematicController.h"
#include "RNJoltInternals.h"
#include "RNJoltWorld.h"

#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace RN
{
	RNDefineMeta(JoltKinematicController, JoltCollisionObject)

	JoltKinematicController::JoltKinematicController(float radius, float height, float stepOffset, JoltWorld *owningWorld) :
		JoltCollisionObject(owningWorld),
		_radius(radius), _height(height), _stepOffset(stepOffset), _objectBelow(nullptr), _isFalling(false)
	{
		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();

		_shape = JoltCapsuleShape::WithRadius(radius, height)->Retain();

		JPH::CharacterVirtualSettings settings;
		settings.mMaxSlopeAngle = JPH::DegreesToRadians(70.0f);
		settings.mMaxStrength = 10.0f;
		settings.mShape = _shape->GetJoltShape();
		settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -radius);
		//settings.mBackFaceMode = sBackFaceMode;
		//settings.mCharacterPadding = sCharacterPadding;
		//settings.mPenetrationRecoverySpeed = sPenetrationRecoverySpeed;
		//settings.mPredictiveContactDistance = sPredictiveContactDistance;
		_controller = new JPH::CharacterVirtual(&settings, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), physics);
		_internals->contactListener.controller = this;
		_controller->SetListener(&_internals->contactListener);
	}

	JoltKinematicController::~JoltKinematicController()
	{
		SafeRelease(_shape);
		delete _controller;
		//if(_callback) delete _callback;
	}

	void JoltKinematicController::Move(const Vector3 &velocity, const Vector3 &gravity, float delta)
	{
		if(delta <= k::EpsilonFloat)
		{
			UpdateGroundState();
			return;
		}

		JoltWorld *world = GetJoltWorld();
		uint16 objectLayer = world->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1);
		JPH::PhysicsSystem *physics = world->GetJoltInstance();
		JPH::Vec3 joltGravity = JoltNativeConversions::ToJoltVector(gravity);

		_controller->SetLinearVelocity(JoltNativeConversions::ToJoltVector(velocity));
		if(_stepOffset > k::EpsilonFloat)
		{
			JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
			updateSettings.mWalkStairsStepUp = JPH::Vec3(0.0f, _stepOffset, 0.0f);
			updateSettings.mStickToFloorStepDown = JPH::Vec3(0.0f, -_stepOffset, 0.0f);
			_controller->ExtendedUpdate(delta, joltGravity, updateSettings, physics->GetDefaultBroadPhaseLayerFilter(objectLayer), physics->GetDefaultLayerFilter(objectLayer), {}, {}, *world->_internals->tempAllocator);
		}
		else
		{
			_controller->Update(delta, joltGravity, physics->GetDefaultBroadPhaseLayerFilter(objectLayer), physics->GetDefaultLayerFilter(objectLayer), {}, {}, *world->_internals->tempAllocator);
		}

		UpdatePosition();
		UpdateGroundState();
	}

	std::vector<JoltContactInfo> JoltKinematicController::SweepTestAll(const Vector3 &direction, const Vector3 &offset) const
	{
		std::vector<JoltContactInfo> hits;

		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();

		JPH::RVec3 baseOffset = JoltNativeConversions::GetAttachmentPosition(this, GetJoltWorld()) + JoltNativeConversions::ToJoltRVec3(offset);
		Quaternion rot = GetWorldRotation();

		JPH::RMat44 worldTransform = JoltNativeConversions::ToJoltRMat44(JoltNativeConversions::ToJoltSceneRotation(rot, GetJoltWorld()), baseOffset);

		//TODO: Limit max distance of raycast or the result

		JPH::RShapeCast castInfo = JPH::RShapeCast::sFromWorldTransform(_shape->GetJoltShape(), JPH::Vec3::sOne(), worldTransform, JoltNativeConversions::ToJoltVector(direction));

		JPH::ShapeCastSettings castSettings; //Defaults seem ok for now!?

		uint16 objectLayer = GetJoltWorld()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1);
		JPH::AllHitCollisionCollector<JPH::CastShapeCollector> results;
		physics->GetNarrowPhaseQuery().CastShape(castInfo, castSettings, baseOffset, results, physics->GetDefaultBroadPhaseLayerFilter(objectLayer), physics->GetDefaultLayerFilter(objectLayer));

		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		for(auto result : results.mHits)
		{
			JoltContactInfo hit;

			JPH::RVec3 position = baseOffset + result.mContactPointOn2; //castInfo.GetPointOnRay(result.mFraction);
			JPH::Vec3 normal;

			{
				JPH::TransformedShape transformedShape = bodyInterface.GetTransformedShape(result.mBodyID2);
				if(!transformedShape.mShape) continue;

				normal = transformedShape.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, position);
				hit.collisionObject = reinterpret_cast<JoltCollisionObject *>(bodyInterface.GetUserData(result.mBodyID2));
				JoltContactInfoShapeData::FillForTransformedShape(hit, transformedShape, result.mSubShapeID2);
			}

			hit.position = JoltNativeConversions::ToPosition(position);
			hit.normal = JoltNativeConversions::ToEngineVector(normal);

			hit.distance = JoltNativeConversions::ToVector3(result.mContactPointOn2).GetLength();

			if(hit.collisionObject) hit.node = hit.collisionObject->GetParent();
			if(hit.node) hit.node->Retain()->Autorelease();

			hits.push_back(hit);
		}

		return hits;
	}

	JoltContactInfo JoltKinematicController::SweepTest(const Vector3 &direction, const Vector3 &offset) const
	{
		JoltContactInfo hit;
		hit.distance = -1.0f;
		hit.node = nullptr;
		hit.collisionObject = nullptr;

		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();

		JPH::RVec3 baseOffset = JoltNativeConversions::GetAttachmentPosition(this, GetJoltWorld()) + JoltNativeConversions::ToJoltRVec3(offset);
		Quaternion rot = GetWorldRotation();

		JPH::RMat44 worldTransform = JoltNativeConversions::ToJoltRMat44(JoltNativeConversions::ToJoltSceneRotation(rot, GetJoltWorld()), baseOffset);

		//TODO: Limit max distance of raycast or the result

		JPH::RShapeCast castInfo = JPH::RShapeCast::sFromWorldTransform(_shape->GetJoltShape(), JPH::Vec3::sOne(), worldTransform, JoltNativeConversions::ToJoltVector(direction));

		JPH::ShapeCastSettings castSettings; //Defaults seem ok for now!?

		uint16 objectLayer = GetJoltWorld()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1);
		JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> result;
		physics->GetNarrowPhaseQuery().CastShape(castInfo, castSettings, baseOffset, result, physics->GetDefaultBroadPhaseLayerFilter(objectLayer), physics->GetDefaultLayerFilter(objectLayer));
		if(!result.HadHit())
		{
			return hit;
		}

		JPH::RVec3 position = baseOffset + result.mHit.mContactPointOn2; //castInfo.GetPointOnRay(result.mHit.mFraction);
		JPH::Vec3 normal;

		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		{
			JPH::TransformedShape transformedShape = bodyInterface.GetTransformedShape(result.mHit.mBodyID2);
			if(!transformedShape.mShape) return hit;

			normal = transformedShape.GetWorldSpaceSurfaceNormal(result.mHit.mSubShapeID2, position);
			hit.collisionObject = reinterpret_cast<JoltCollisionObject *>(bodyInterface.GetUserData(result.mHit.mBodyID2));
			JoltContactInfoShapeData::FillForTransformedShape(hit, transformedShape, result.mHit.mSubShapeID2);
		}

		hit.position = JoltNativeConversions::ToPosition(position);
		hit.normal = JoltNativeConversions::ToEngineVector(normal);

		hit.distance = JoltNativeConversions::ToVector3(result.mHit.mContactPointOn2).GetLength();

		if(hit.collisionObject) hit.node = hit.collisionObject->GetParent();
		if(hit.node) hit.node->Retain()->Autorelease();

		return hit;
	}

	JoltContactInfo JoltKinematicController::OverlapTest() const
	{
		JoltContactInfo contact;
		contact.distance = -1.0f;
		contact.node = nullptr;
		contact.collisionObject = nullptr;

		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();

		JPH::RVec3 baseOffset = JoltNativeConversions::GetAttachmentPosition(this, GetJoltWorld());
		JoltPosition position = JoltNativeConversions::ToPosition(baseOffset);
		Quaternion rotation = GetWorldRotation();

		JPH::RMat44 worldTransform = JoltNativeConversions::ToJoltRMat44(JoltNativeConversions::ToJoltSceneRotation(rotation, GetJoltWorld()), baseOffset);
		JPH::CollideShapeSettings collideSettings; //Defaults seem ok for now!?

		JPH::ClosestHitCollisionCollector<JPH::CollideShapeCollector> results;
		uint16 objectLayer = GetJoltWorld()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1);
		physics->GetNarrowPhaseQuery().CollideShape(_shape->GetJoltShape(), JPH::Vec3::sOne(), worldTransform.PreTranslated(_shape->GetJoltShape()->GetCenterOfMass()), collideSettings, baseOffset, results, physics->GetDefaultBroadPhaseLayerFilter(objectLayer), physics->GetDefaultLayerFilter(objectLayer));

		if(!results.HadHit())
		{
			return contact;
		}

		contact.distance = 0.0f;
		contact.position = position;
		contact.node = nullptr;
		contact.collisionObject = nullptr;

		{
			JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
			JPH::TransformedShape transformedShape = bodyInterface.GetTransformedShape(results.mHit.mBodyID2);
			if(!transformedShape.mShape) return contact;

			contact.collisionObject = reinterpret_cast<JoltCollisionObject *>(bodyInterface.GetUserData(results.mHit.mBodyID2));
			JoltContactInfoShapeData::FillForTransformedShape(contact, transformedShape, results.mHit.mSubShapeID2);
		}
		if(contact.collisionObject) contact.node = contact.collisionObject->GetParent();
		if(contact.node) contact.node->Retain()->Autorelease();

		return contact;
	}

	std::vector<JoltContactInfo> JoltKinematicController::OverlapTestAll() const
	{
		std::vector<JoltContactInfo> hits;

		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();

		JPH::RVec3 baseOffset = JoltNativeConversions::GetAttachmentPosition(this, GetJoltWorld());
		JoltPosition position = JoltNativeConversions::ToPosition(baseOffset);
		Quaternion rotation = GetWorldRotation();

		JPH::RMat44 worldTransform = JoltNativeConversions::ToJoltRMat44(JoltNativeConversions::ToJoltSceneRotation(rotation, GetJoltWorld()), baseOffset);
		JPH::CollideShapeSettings collideSettings; //Defaults seem ok for now!?

		JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> results;
		uint16 objectLayer = GetJoltWorld()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1);
		physics->GetNarrowPhaseQuery().CollideShape(_shape->GetJoltShape(), JPH::Vec3::sOne(), worldTransform.PreTranslated(_shape->GetJoltShape()->GetCenterOfMass()), collideSettings, baseOffset, results, physics->GetDefaultBroadPhaseLayerFilter(objectLayer), physics->GetDefaultLayerFilter(objectLayer));

		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		for(auto result : results.mHits)
		{
			JoltContactInfo hit;
			hit.distance = 0.0f;
			hit.position = position;
			hit.node = nullptr;
			hit.collisionObject = nullptr;

			{
				JPH::TransformedShape transformedShape = bodyInterface.GetTransformedShape(result.mBodyID2);
				if(!transformedShape.mShape) continue;

				hit.collisionObject = reinterpret_cast<JoltCollisionObject *>(bodyInterface.GetUserData(result.mBodyID2));
				JoltContactInfoShapeData::FillForTransformedShape(hit, transformedShape, result.mSubShapeID2);
			}
			if(hit.collisionObject) hit.node = hit.collisionObject->GetParent();
			if(hit.node) hit.node->Retain()->Autorelease();

			hits.push_back(hit);
		}

		return hits;
	}

	bool JoltKinematicController::Resize(float height, bool checkIfBlocked)
	{
		float heightDifference = height - _height;
		if(((heightDifference < 0.0f) ? -heightDifference : heightDifference) < k::EpsilonFloat) return true;

		uint16 objectLayer = GetJoltWorld()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1);
		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();

		JoltShape *newShape = JoltCapsuleShape::WithRadius(_radius, height);
		bool didSetShape = _controller->SetShape(newShape->GetJoltShape(), checkIfBlocked ? 0.01f : 1000000.0f, physics->GetDefaultBroadPhaseLayerFilter(objectLayer), physics->GetDefaultLayerFilter(objectLayer), {}, {}, *GetJoltWorld()->_internals->tempAllocator);
		if(!didSetShape) return false;

		SafeRelease(_shape);
		_shape = newShape->Retain();
		_height = height;
		return true;
	}

	void JoltKinematicController::SetCollisionFilter(uint32 group, uint32 mask)
	{
		JoltCollisionObject::SetCollisionFilter(group, mask);

		//No need to do anything here, values will just be used by the actual methods that do the work.
	}

	Vector3 JoltKinematicController::GetFeetOffset() const
	{
		return Vector3(0.0f, -(_height * 0.5f + _radius), 0.0f);
	}

	void JoltKinematicController::UpdateGroundState()
	{
		if(!_controller->IsSupported())
		{
			_objectBelow = nullptr;
			_isFalling = true;
			return;
		}

		JoltCollisionObject *collisionObject = reinterpret_cast<JoltCollisionObject *>(_controller->GetGroundUserData());
		_objectBelow = collisionObject ? collisionObject->GetParent() : nullptr;
		if(_objectBelow) _objectBelow->Retain()->Autorelease();
		_isFalling = false;
	}

	void JoltKinematicController::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		JoltCollisionObject::DidUpdate(changeSet);

		if(changeSet & SceneNode::ChangeSet::Position)
		{
			_controller->SetPosition(JoltNativeConversions::GetAttachmentPosition(this, -_positionOffset, GetJoltWorld()));
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent())
			{
				_controller->SetPosition(JoltNativeConversions::GetAttachmentPosition(this, -_positionOffset, GetJoltWorld()));
			}

			_owner = GetParent();
		}
	}

	void JoltKinematicController::UpdatePosition()
	{
		if(!GetParent())
		{
			return;
		}

		JPH::RVec3 position = _controller->GetPosition();
		JoltNativeConversions::SetAttachmentPosition(this, position, _positionOffset, GetJoltWorld());
	}
} // namespace RN
