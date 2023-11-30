//
//  RNJoltKinematicController.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltKinematicController.h"
#include "RNJoltWorld.h"
#include "RNJoltInternals.h"

#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace RN
{
	RNDefineMeta(JoltKinematicController, JoltCollisionObject)
		
	JoltKinematicController::JoltKinematicController(float radius, float height, JoltMaterial *material, float stepOffset) : _fallSpeed(0.0f), _objectBelow(nullptr), _isFalling(false)
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		
		_shape = JoltCapsuleShape::WithRadius(radius, height, material)->Retain();
		
		JPH::CharacterVirtualSettings settings;
		settings.mMaxSlopeAngle = 70.0f;
		settings.mMaxStrength = 10.0f;
		settings.mShape = _shape->GetJoltShape();
		//settings->mBackFaceMode = sBackFaceMode;
		//settings->mCharacterPadding = sCharacterPadding;
		//settings->mPenetrationRecoverySpeed = sPenetrationRecoverySpeed;
		//settings->mPredictiveContactDistance = sPredictiveContactDistance;
		settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -radius); // Accept contacts that touch the lower sphere of the capsule
		_controller = new JPH::CharacterVirtual(&settings, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), physics);
		//_controller->SetListener(this);

		//_callback = new JoltKinematicControllerCallback();
	}
	
	JoltKinematicController::~JoltKinematicController()
	{
		SafeRelease(_shape);
		delete _controller;
		//if(_callback) delete _callback;
	}
		
	void JoltKinematicController::Move(const Vector3 &direction, float delta)
	{
		if(delta < k::EpsilonFloat || direction.GetLength() < k::EpsilonFloat)
		{
			return;
		}

		uint16 objectLayer = JoltWorld::GetSharedInstance()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1);
		
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		
		_controller->SetLinearVelocity(JPH::Vec3Arg(direction.x, direction.y, direction.z));
		_controller->Update(delta, physics->GetGravity(), physics->GetDefaultBroadPhaseLayerFilter(objectLayer), physics->GetDefaultLayerFilter(objectLayer), {}, {}, *JoltWorld::GetSharedInstance()->_internals->tempAllocator);
		
		UpdatePosition();
	}

	void JoltKinematicController::Gravity(float gforce, float delta)
	{
		JoltContactInfo contact = SweepTest(RN::Vector3(0.0f, -10000.0f, 0.0f));
		float groundDistance = _controller->GetPosition().GetY() - contact.position.y + GetFeetOffset().y;
		float fallDistance = 0.0f;
		_fallSpeed += gforce * delta;
		if((groundDistance + _fallSpeed * delta) < k::EpsilonFloat || (!_isFalling && groundDistance + gforce * 2.0f * delta < 0.0f))
		{
			_fallSpeed = 0.0f;
			_isFalling = false;
			fallDistance = -groundDistance;
		}
		else
		{
			fallDistance = _fallSpeed * delta;
			_isFalling = true;
		}
		
		_objectBelow = contact.node;
		if(std::abs(fallDistance) > k::EpsilonFloat)
		{
			Move(RN::Vector3(0.0f, fallDistance, 0.0f), delta);
		}
	}

	std::vector<JoltContactInfo> JoltKinematicController::SweepTestAll(const Vector3 &direction, const Vector3 &offset) const
	{
		std::vector<JoltContactInfo> hits;
		
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		
		Vector3 pos = GetWorldPosition() + offset;
		Quaternion rot = GetWorldRotation();
		
		JPH::Mat44 worldTransform = JPH::Mat44::sRotationTranslation(JPH::QuatArg(rot.x, rot.y, rot.z, rot.w), JPH::Vec3Arg(pos.x, pos.y, pos.z));
		
		//TODO: Limit max distance of raycast or the result
		
		JPH::RShapeCast castInfo = JPH::RShapeCast::sFromWorldTransform(_shape->GetJoltShape(), JPH::Vec3Arg(1, 1, 1), worldTransform, JPH::Vec3Arg(direction.x, direction.y, direction.z));
		
		JPH::ShapeCastSettings castSettings; //Defaults seem ok for now!?
		
		uint16 objectLayer = JoltWorld::GetSharedInstance()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1);
		JPH::AllHitCollisionCollector<JPH::CastShapeCollector> results;
		physics->GetNarrowPhaseQuery().CastShape(castInfo, castSettings, JPH::RVec3Arg(0, 0, 0), results, physics->GetDefaultBroadPhaseLayerFilter(objectLayer), physics->GetDefaultLayerFilter(objectLayer));
		
		for(auto result : results.mHits)
		{
			JoltContactInfo hit;
			
			JPH::Vec3 position = castInfo.GetPointOnRay(result.mFraction);
			JPH::Vec3 normal;

			// Scoped lock
			{
				JPH::BodyLockRead lock(physics->GetBodyLockInterface(), result.mBodyID2);
				if(lock.Succeeded()) // bodyID may no longer be valid
				{
					const JPH::Body &body = lock.GetBody();
					normal = body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, position);
					hit.collisionObject = reinterpret_cast<JoltCollisionObject*>(body.GetUserData());
				}
				else
				{
					continue;
				}
			}
			
			hit.position.x = position.GetX();
			hit.position.y = position.GetY();
			hit.position.z = position.GetZ();
			
			hit.normal.x = normal.GetX();
			hit.normal.y = normal.GetY();
			hit.normal.z = normal.GetZ();

			hit.distance = pos.GetDistance(hit.position);
			
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
		
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		
		Vector3 pos = GetWorldPosition() + offset;
		Quaternion rot = GetWorldRotation();
		
		JPH::Mat44 worldTransform = JPH::Mat44::sRotationTranslation(JPH::QuatArg(rot.x, rot.y, rot.z, rot.w), JPH::Vec3Arg(pos.x, pos.y, pos.z));
		
		//TODO: Limit max distance of raycast or the result
		
		JPH::RShapeCast castInfo = JPH::RShapeCast::sFromWorldTransform(_shape->GetJoltShape(), JPH::Vec3Arg(1, 1, 1), worldTransform, JPH::Vec3Arg(direction.x, direction.y, direction.z));
		
		JPH::ShapeCastSettings castSettings; //Defaults seem ok for now!?
		
		uint16 objectLayer = JoltWorld::GetSharedInstance()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1);
		JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> result;
		physics->GetNarrowPhaseQuery().CastShape(castInfo, castSettings, JPH::RVec3Arg(0, 0, 0), result, physics->GetDefaultBroadPhaseLayerFilter(objectLayer), physics->GetDefaultLayerFilter(objectLayer));
		if(!result.HadHit())
		{
			return hit;
		}
		
		JPH::Vec3 position = castInfo.GetPointOnRay(result.mHit.mFraction);
		JPH::Vec3 normal;

		// Scoped lock
		{
			JPH::BodyLockRead lock(physics->GetBodyLockInterface(), result.mHit.mBodyID2);
			if(lock.Succeeded()) // bodyID may no longer be valid
			{
				const JPH::Body &body = lock.GetBody();
				normal = body.GetWorldSpaceSurfaceNormal(result.mHit.mSubShapeID2, position);
				hit.collisionObject = reinterpret_cast<JoltCollisionObject*>(body.GetUserData());
			}
			else
			{
				return hit;
			}
		}
		
		hit.position.x = position.GetX();
		hit.position.y = position.GetY();
		hit.position.z = position.GetZ();
		
		hit.normal.x = normal.GetX();
		hit.normal.y = normal.GetY();
		hit.normal.z = normal.GetZ();

		hit.distance = pos.GetDistance(hit.position);
		
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
		
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		
		Vector3 position = GetWorldPosition();
		Quaternion rotation = GetWorldRotation();

		JPH::Mat44 worldTransform = JPH::Mat44::sRotationTranslation(JPH::QuatArg(rotation.x, rotation.y, rotation.z, rotation.w), JPH::Vec3Arg(position.x, position.y, position.z));
		JPH::CollideShapeSettings collideSettings; //Defaults seem ok for now!?
		
		JPH::ClosestHitCollisionCollector<JPH::CollideShapeCollector> results;
		uint16 objectLayer = JoltWorld::GetSharedInstance()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1);
		physics->GetNarrowPhaseQuery().CollideShape(_shape->GetJoltShape(), JPH::Vec3Arg(1, 1, 1), worldTransform.PreTranslated(_shape->GetJoltShape()->GetCenterOfMass()), collideSettings, JPH::RVec3Arg(0, 0, 0), results, physics->GetDefaultBroadPhaseLayerFilter(objectLayer), physics->GetDefaultLayerFilter(objectLayer));
		
		if(!results.HadHit())
		{
			return contact;
		}
		
		contact.distance = 0.0f;
		contact.position = position;
		contact.node = nullptr;
		contact.collisionObject = nullptr;
		
		contact.collisionObject = reinterpret_cast<JoltCollisionObject*>(physics->GetBodyInterface().GetUserData(results.mHit.mBodyID2));
		if(contact.collisionObject) contact.node = contact.collisionObject->GetParent();
		if(contact.node) contact.node->Retain()->Autorelease();
		
		return contact;
	}

	std::vector<JoltContactInfo> JoltKinematicController::OverlapTestAll() const
	{
		std::vector<JoltContactInfo> hits;
		
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		
		Vector3 position = GetWorldPosition();
		Quaternion rotation = GetWorldRotation();

		JPH::Mat44 worldTransform = JPH::Mat44::sRotationTranslation(JPH::QuatArg(rotation.x, rotation.y, rotation.z, rotation.w), JPH::Vec3Arg(position.x, position.y, position.z));
		JPH::CollideShapeSettings collideSettings; //Defaults seem ok for now!?
		
		JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> results;
		uint16 objectLayer = JoltWorld::GetSharedInstance()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1);
		physics->GetNarrowPhaseQuery().CollideShape(_shape->GetJoltShape(), JPH::Vec3Arg(1, 1, 1), worldTransform.PreTranslated(_shape->GetJoltShape()->GetCenterOfMass()), collideSettings, JPH::RVec3Arg(0, 0, 0), results, physics->GetDefaultBroadPhaseLayerFilter(objectLayer), physics->GetDefaultLayerFilter(objectLayer));
		
		for(auto result : results.mHits)
		{
			JoltContactInfo hit;
			hit.distance = 0.0f;
			hit.position = position;
			hit.node = nullptr;
			hit.collisionObject = nullptr;
			
			hit.collisionObject = reinterpret_cast<JoltCollisionObject*>(physics->GetBodyInterface().GetUserData(result.mBodyID2));
			if(hit.collisionObject) hit.node = hit.collisionObject->GetParent();
			if(hit.node) hit.node->Retain()->Autorelease();

			hits.push_back(hit);
		}
		
		return hits;
	}

	bool JoltKinematicController::Resize(float height, bool checkIfBlocked)
	{
		/*bool isBlocked = false;
		float oldHeight = _controller->getHeight();
		if(checkIfBlocked && height > oldHeight)
		{
			float radius = _controller->getRadius();
			float dh = std::max(height - oldHeight - 2.0f * radius, 0.0f);
			Jolt::PxCapsuleGeometry geom(radius, dh * 0.5f);
			float temporaryShapeHeight = dh + radius * 2.0f;
			float offset = height - temporaryShapeHeight * 0.5f;

			Jolt::PxExtendedVec3 position = _controller->getPosition();
			Jolt::PxVec3 pos((float)position.x, (float)position.y + offset, (float)position.z);
			Jolt::PxQuat orientation(k::Pi_2, Jolt::PxVec3(0.0f, 0.0f, 1.0f));

			Jolt::PxFilterData filterData;
			filterData.word0 = _collisionFilterGroup;
			filterData.word1 = _collisionFilterMask;
			filterData.word2 = _collisionFilterID;
			filterData.word3 = _collisionFilterIgnoreID;
			Jolt::PxOverlapBuffer hit;
			JoltQueryFilterCallback filterCallback;
			JoltWorld *JoltWorld = JoltWorld::GetSharedInstance();
			if(JoltWorld->GetJoltScene()->overlap(geom, Jolt::PxTransform(pos, orientation), hit, Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eANY_HIT|Jolt::PxQueryFlag::eSTATIC|Jolt::PxQueryFlag::eDYNAMIC|Jolt::PxQueryFlag::ePREFILTER), &filterCallback)) isBlocked = true;
		}
		
		if(!isBlocked)
		{
			_controller->resize(height);
		}
		
		return !isBlocked;*/
		
		return false;
	}

	void JoltKinematicController::SetCollisionFilter(uint32 group, uint32 mask)
	{
		JoltCollisionObject::SetCollisionFilter(group, mask);
		
		//No need to do anything here, values will just be used by the actual methods that do the work.
	}

	Vector3 JoltKinematicController::GetFeetOffset() const
	{
/*		Jolt::PxVec3 footPosition = Jolt::toVec3(_controller->getFootPosition());
		Jolt::PxVec3 position = Jolt::toVec3(_controller->getPosition());
		Jolt::PxVec3 offset = footPosition - position;*/
		return Vector3();//Vector3(offset.x, offset.y, offset.z);
	}

	void JoltKinematicController::Jump(float force)
	{
		_fallSpeed = force;
		_isFalling = true;
	}

	void JoltKinematicController::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		JoltCollisionObject::DidUpdate(changeSet);
		
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			Vector3 position = GetWorldPosition() - _positionOffset;
			_controller->SetPosition(JPH::RVec3Arg(position.x, position.y, position.z));
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent())
			{
				Vector3 position = GetWorldPosition() - _positionOffset;
				_controller->SetPosition(JPH::RVec3Arg(position.x, position.y, position.z));
			}

			_owner = GetParent();
		}
	}

	void JoltKinematicController::UpdatePosition()
	{
		if(!_owner)
		{
			return;
		}
		
		JPH::RVec3 position = _controller->GetPosition();
		SetWorldPosition(Vector3(position.GetX(), position.GetY(), position.GetZ()) + _positionOffset);
	}
}
