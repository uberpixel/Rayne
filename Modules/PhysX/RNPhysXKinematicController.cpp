//
//  RNPhysXKinematicController.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXKinematicController.h"
#include "RNPhysXWorld.h"
#include "RNPhysXInternals.h"

#include "PxPhysicsAPI.h"

namespace RN
{
	RNDefineMeta(PhysXKinematicController, PhysXCollisionObject)
		
	PhysXKinematicController::PhysXKinematicController(float radius, float height, PhysXMaterial *material, float stepOffset) : _fallSpeed(0.0f), _objectBelow(nullptr), _isFalling(false)
	{
		_material = material->Retain();

		_callback = new PhysXKinematicControllerCallback();

		physx::PxCapsuleControllerDesc desc;
		desc.height = height;
		desc.radius = radius;
		desc.position.set(0.0f, 10.0, 0.0f);
		desc.stepOffset = stepOffset;
		desc.material = _material->GetPhysXMaterial();
		desc.reportCallback = _callback;
		desc.behaviorCallback = nullptr;//_callback;
		desc.userData = this;

		physx::PxControllerManager *manager = PhysXWorld::GetSharedInstance()->GetPhysXControllerManager();
		_controller = static_cast<physx::PxCapsuleController*>(manager->createController(desc));
	}
	
	PhysXKinematicController::~PhysXKinematicController()
	{
		_controller->release();
		if(_callback) delete _callback;
	}
		
	void PhysXKinematicController::Move(const Vector3 &direction, float delta)
	{
		if(delta < k::EpsilonFloat || direction.GetLength() < k::EpsilonFloat)
		{
			return;
		}

		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;
		physx::PxControllerFilters controllerFilter(&filterData, _callback, _callback);
		physx::PxControllerCollisionFlags collisionFlags = _controller->move(physx::PxVec3(direction.x, direction.y, direction.z), 0.0f, delta, controllerFilter);

		UpdatePosition();
	}

	void PhysXKinematicController::Gravity(float gforce, float delta)
	{
		PhysXContactInfo contact = SweepTest(RN::Vector3(0.0f, -10000.0f, 0.0f));
		float groundDistance = _controller->getPosition().y - contact.position.y + GetFeetOffset().y;
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

	std::vector<PhysXContactInfo> PhysXKinematicController::SweepTestAll(const Vector3 &direction, const Vector3 &offset) const
	{
		const physx::PxExtendedVec3 &position = _controller->getPosition();
		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		float length = direction.GetLength();
		Vector3 normalizedDirection = direction.GetNormalized();
		const physx::PxU32 bufferSize = 2048;
		physx::PxSweepHit hitBuffer[bufferSize];
		physx::PxSweepBuffer hit(hitBuffer, bufferSize);
		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;
		PhysXQueryFilterCallback filterCallback;
		physx::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		Quaternion orientation(RN::Vector3(0.0f, 0.0f, 90.0f));
		scene->sweep(shape->getGeometry().any(), physx::PxTransform(physx::PxVec3(position.x + offset.x, position.y + offset.y, position.z + offset.z), physx::PxQuat(orientation.x, orientation.y, orientation.z, orientation.w)), physx::PxVec3(normalizedDirection.x, normalizedDirection.y, normalizedDirection.z), length, hit, physx::PxHitFlags(physx::PxHitFlag::eDEFAULT), physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eDYNAMIC|physx::PxQueryFlag::eSTATIC|physx::PxQueryFlag::ePREFILTER|physx::PxQueryFlag::eNO_BLOCK), &filterCallback);
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, true);
		
		std::vector<PhysXContactInfo> contacts;

		if(hit.getNbTouches() == 0)
			return contacts;
		
		for(uint32 i = 0; i < hit.nbTouches; i++)
		{
			physx::PxSweepHit currentHit = hit.touches[i];
			
			PhysXContactInfo contact;
			contact.distance = currentHit.distance;
			contact.node = nullptr;
			contact.collisionObject = nullptr;

			contact.position = Vector3(currentHit.position.x, currentHit.position.y, currentHit.position.z);
			contact.normal = Vector3(currentHit.normal.x, currentHit.normal.y, currentHit.normal.z);
			if(currentHit.actor)
			{
				PhysXCollisionObject *collisionObject = static_cast<PhysXCollisionObject*>(currentHit.actor->userData);
				contact.collisionObject = collisionObject;
				if(collisionObject->GetParent())
				{
					contact.node = collisionObject->GetParent();
					if(contact.node) contact.node->Retain()->Autorelease();
				}
			}

			contacts.push_back(contact);
		}
		
		return contacts;
	}

	PhysXContactInfo PhysXKinematicController::SweepTest(const Vector3 &direction, const Vector3 &offset) const
	{
		const physx::PxExtendedVec3 &position = _controller->getPosition();
		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		float length = direction.GetLength();
		Vector3 normalizedDirection = direction.GetNormalized();
		physx::PxSweepBuffer hit;
		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;
		PhysXQueryFilterCallback filterCallback;
		physx::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		Quaternion orientation(RN::Vector3(0.0f, 0.0f, 90.0f));
		bool didHit = scene->sweep(shape->getGeometry().any(), physx::PxTransform(physx::PxVec3(position.x + offset.x, position.y + offset.y, position.z + offset.z), physx::PxQuat(orientation.x, orientation.y, orientation.z, orientation.w)), physx::PxVec3(normalizedDirection.x, normalizedDirection.y, normalizedDirection.z), length, hit, physx::PxHitFlags(physx::PxHitFlag::eDEFAULT), physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eDYNAMIC|physx::PxQueryFlag::eSTATIC|physx::PxQueryFlag::ePREFILTER), &filterCallback);
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, true);
		
		PhysXContactInfo contact;
		contact.distance = -1.0f;
		contact.node = nullptr;
		contact.collisionObject = nullptr;

		if(!didHit)
			return contact;

		physx::PxSweepHit closestHit = hit.block;
		contact.distance = closestHit.distance;
		contact.position = Vector3(closestHit.position.x, closestHit.position.y, closestHit.position.z);
		contact.normal = Vector3(closestHit.normal.x, closestHit.normal.y, closestHit.normal.z);
		if(closestHit.actor)
		{
			PhysXCollisionObject *collisionObject = static_cast<PhysXCollisionObject*>(closestHit.actor->userData);
			contact.collisionObject = collisionObject;
			if(collisionObject->GetParent())
			{
				contact.node = collisionObject->GetParent();
				if(contact.node) contact.node->Retain()->Autorelease();
			}
		}
		return contact;
	}

	PhysXContactInfo PhysXKinematicController::OverlapTest() const
	{
		const physx::PxExtendedVec3 &position = _controller->getPosition();
		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		physx::PxOverlapBuffer hit;
		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;
		PhysXQueryFilterCallback filterCallback;
		physx::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		Quaternion orientation(RN::Vector3(0.0f, 0.0f, 90.0f));
		scene->overlap(shape->getGeometry().any(), physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(orientation.x, orientation.y, orientation.z, orientation.w)), hit, physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eDYNAMIC|physx::PxQueryFlag::eSTATIC|physx::PxQueryFlag::ePREFILTER), &filterCallback);
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, true);
		
		PhysXContactInfo contact;
		contact.distance = -1.0f;
		contact.node = nullptr;
		contact.collisionObject = nullptr;

		if(hit.getNbAnyHits() == 0)
			return contact;

		physx::PxOverlapHit closestHit = hit.getAnyHit(0);
		contact.distance = 0.0f;
		contact.position = Vector3(position.x, position.y, position.z);
		contact.normal = Vector3(0.0f, 0.0f, 0.0f);
		if(closestHit.actor)
		{
			PhysXCollisionObject *collisionObject = static_cast<PhysXCollisionObject*>(closestHit.actor->userData);
			contact.collisionObject = collisionObject;
			if(collisionObject->GetParent())
			{
				contact.node = collisionObject->GetParent();
				if(contact.node) contact.node->Retain()->Autorelease();
			}
		}
		return contact;
	}

	std::vector<PhysXContactInfo> PhysXKinematicController::OverlapTestAll() const
	{
		const physx::PxExtendedVec3 &position = _controller->getPosition();
		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		physx::PxOverlapHit hitBuffer[256];
		physx::PxOverlapBuffer hit(hitBuffer, 255);
		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;
		PhysXQueryFilterCallback filterCallback;
		physx::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		Quaternion orientation(RN::Vector3(0.0f, 0.0f, 90.0f));
		scene->overlap(shape->getGeometry().any(), physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(orientation.x, orientation.y, orientation.z, orientation.w)), hit, physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eDYNAMIC|physx::PxQueryFlag::eSTATIC|physx::PxQueryFlag::ePREFILTER|physx::PxQueryFlag::eNO_BLOCK), &filterCallback);
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, true);

		std::vector<PhysXContactInfo> contacts;

		if(hit.getNbAnyHits() == 0)
			return contacts;
		
		for(uint32 i = 0; i < hit.getNbAnyHits(); i++)
		{
			physx::PxOverlapHit currentHit = hit.getAnyHit(i);
			
			PhysXContactInfo contact;
			contact.distance = 0.0f;
			contact.node = nullptr;
			contact.collisionObject = nullptr;

			contact.position = Vector3(position.x, position.y, position.z);
			contact.normal = Vector3(0.0f, 0.0f, 0.0f);
			if(currentHit.actor)
			{
				PhysXCollisionObject *collisionObject = static_cast<PhysXCollisionObject*>(currentHit.actor->userData);
				contact.collisionObject = collisionObject;
				if(collisionObject->GetParent())
				{
					contact.node = collisionObject->GetParent();
					if(contact.node) contact.node->Retain()->Autorelease();
				}
			}

			contacts.push_back(contact);
		}

        return contacts;
	}

	bool PhysXKinematicController::Resize(float height, bool checkIfBlocked)
	{
		bool isBlocked = false;
		float oldHeight = _controller->getHeight();
		if(checkIfBlocked && height > oldHeight)
		{
			float radius = _controller->getRadius();
			float dh = std::max(height - oldHeight - 2.0f * radius, 0.0f);
			physx::PxCapsuleGeometry geom(radius, dh * 0.5f);
			float temporaryShapeHeight = dh + radius * 2.0f;
			float offset = height - temporaryShapeHeight * 0.5f;

			physx::PxExtendedVec3 position = _controller->getPosition();
			physx::PxVec3 pos((float)position.x, (float)position.y + offset, (float)position.z);
			physx::PxQuat orientation(k::Pi_2, physx::PxVec3(0.0f, 0.0f, 1.0f));

			physx::PxFilterData filterData;
			filterData.word0 = _collisionFilterGroup;
			filterData.word1 = _collisionFilterMask;
			filterData.word2 = _collisionFilterID;
			filterData.word3 = _collisionFilterIgnoreID;
			physx::PxOverlapBuffer hit;
			PhysXQueryFilterCallback filterCallback;
			PhysXWorld *physXWorld = PhysXWorld::GetSharedInstance();
			if(physXWorld->GetPhysXScene()->overlap(geom, physx::PxTransform(pos, orientation), hit, physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eANY_HIT|physx::PxQueryFlag::eSTATIC|physx::PxQueryFlag::eDYNAMIC|physx::PxQueryFlag::ePREFILTER), &filterCallback)) isBlocked = true;
		}
		
		if(!isBlocked)
		{
			_controller->resize(height);
		}
		
		return !isBlocked;
	}

	void PhysXKinematicController::SetCollisionFilter(uint32 group, uint32 mask)
	{
		PhysXCollisionObject::SetCollisionFilter(group, mask);

		physx::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);

		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;
		shape->setSimulationFilterData(filterData);
		shape->setQueryFilterData(filterData);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);

//		_controller->invalidateCache();
	}

	Vector3 PhysXKinematicController::GetFeetOffset() const
	{
		physx::PxVec3 footPosition = physx::toVec3(_controller->getFootPosition());
		physx::PxVec3 position = physx::toVec3(_controller->getPosition());
		physx::PxVec3 offset = footPosition - position;
		return Vector3(offset.x, offset.y, offset.z);
	}

/*	void BulletKinematicController::SetFallSpeed(float speed)
	{
		_controller->setFallSpeed(speed);
	}
	void BulletKinematicController::SetJumpSpeed(float speed)
	{
		_controller->setJumpSpeed(speed);
	}
	void BulletKinematicController::SetMaxJumpHeight(float maxHeight)
	{
		_controller->setMaxJumpHeight(maxHeight);
	}
	void BulletKinematicController::SetMaxSlope(float maxSlope)
	{
		_controller->setMaxSlope(maxSlope);
	}
	void BulletKinematicController::SetGravity(float gravity)
	{
		_controller->setGravity(btVector3(0.0f, gravity, 0.0f));
	}
		
		
	bool BulletKinematicController::IsOnGround()
	{
		return _controller->canJump();
	}
	void BulletKinematicController::Jump()
	{
		_controller->jump();
	}*/

	void PhysXKinematicController::Jump(float force)
	{
		_fallSpeed = force;
		_isFalling = true;
	}
		
	void PhysXKinematicController::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		PhysXCollisionObject::DidUpdate(changeSet);
			
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			Vector3 position = GetParent()->GetWorldPosition() - _positionOffset;
			_controller->setPosition(physx::PxExtendedVec3(position.x, position.y, position.z));
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent())
			{
				Vector3 position = GetParent()->GetWorldPosition() - _positionOffset;
				_controller->setPosition(physx::PxExtendedVec3(position.x, position.y, position.z));
			}

			_owner = GetParent();
		}
	}

	void PhysXKinematicController::UpdatePosition()
	{
		if(!_owner)
		{
			return;
		}

		const physx::PxExtendedVec3 &position = _controller->getPosition();
		SetWorldPosition(Vector3(position.x, position.y, position.z) + _positionOffset);
	}
}
