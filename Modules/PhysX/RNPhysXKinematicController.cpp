//
//  RNPhysXKinematicController.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXKinematicController.h"
#include "RNPhysXWorld.h"

#include "PxPhysicsAPI.h"

namespace RN
{
	RNDefineMeta(PhysXKinematicController, PhysXCollisionObject)
		
	PhysXKinematicController::PhysXKinematicController(float radius, float height, PhysXMaterial *material) : _gravity(0.0f), _didUpdatePosition(false)
	{
		_material = material->Retain();

		physx::PxCapsuleControllerDesc desc;
		desc.height = height;
		desc.radius = radius;
		desc.position.set(0.0f, 10.0, 0.0f);
		desc.material = _material->GetPhysXMaterial();

		physx::PxControllerManager *manager = PhysXWorld::GetSharedInstance()->GetPhysXControllerManager();
		_controller = manager->createController(desc);
	}
		
	PhysXKinematicController::~PhysXKinematicController()
	{
		_controller->release();
	}
		
	void PhysXKinematicController::Move(const Vector3 &direction, float delta)
	{
		if(delta < k::EpsilonFloat)
		{
			return;
		}

		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterMask;
		physx::PxControllerFilters controllerFilter(&filterData);
		physx::PxControllerCollisionFlags collisionFlags = _controller->move(physx::PxVec3(direction.x, direction.y, direction.z), 0.0f, delta, controllerFilter);

		_didUpdatePosition = true;
		const physx::PxExtendedVec3 &position = _controller->getPosition();
		GetParent()->SetWorldPosition(Vector3(position.x, position.y, position.z) + offset);
	}

	void PhysXKinematicController::Gravity(float gforce, float delta)
	{
		float groundDistance = SweepTest(RN::Vector3(0.0f, -10000.0f, 0.0f));
		groundDistance += GetFeetOffset().y+_controller->getContactOffset()*2.0f;
		if(groundDistance < 0.4f && _gravity < k::EpsilonFloat)
		{
			_gravity = 0.0f;
		}
		else
		{
			_gravity += gforce*delta;
			if(_gravity < groundDistance)
			{
				groundDistance = -_gravity*delta;
			}
			else
			{
				_gravity = 0.0f;
			}
		}

		if(groundDistance > 0.0f)
			Move(RN::Vector3(0.0f, -groundDistance, 0.0f), delta);
	}

	float PhysXKinematicController::SweepTest(const Vector3 &direction, const Vector3 &offset) const
	{
		const physx::PxExtendedVec3 &position = _controller->getPosition();
		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		float length = direction.GetLength();
		Vector3 normalizedDirection = direction.GetNormalized();
		physx::PxSweepBuffer hit;
		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterMask;
		physx::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		scene->sweep(shape->getGeometry().capsule(), physx::PxTransform(physx::PxVec3(position.x + offset.x, position.y + offset.y, position.z + offset.z)), physx::PxVec3(normalizedDirection.x, normalizedDirection.y, normalizedDirection.z), length, hit, physx::PxHitFlags(physx::PxHitFlag::eDEFAULT), physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eDYNAMIC|physx::PxQueryFlag::eSTATIC));
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, true);

		if(hit.getNbAnyHits() == 0)
			return 0.0f;

		return hit.getAnyHit(0).distance;
	}

	void PhysXKinematicController::SetCollisionFilter(uint32 group, uint32 mask)
	{
		PhysXCollisionObject::SetCollisionFilter(group, mask);

		physx::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);

		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		shape->setSimulationFilterData(filterData);
		shape->setQueryFilterData(filterData);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);

//		_controller->invalidateCache();
	}

	Vector3 PhysXKinematicController::GetFeetOffset() const
	{
		physx::PxVec3 offset = _controller->getFootPosition() - _controller->getPosition();
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
		
	void PhysXKinematicController::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		PhysXCollisionObject::DidUpdate(changeSet);
			
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			if(!_didUpdatePosition)
			{
				Vector3 position = GetParent()->GetWorldPosition() - offset;
				_controller->setPosition(physx::PxExtendedVec3(position.x, position.y, position.z));
			}

			_didUpdatePosition = false;
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent())
			{
				Vector3 position = GetParent()->GetWorldPosition() - offset;
				_controller->setPosition(physx::PxExtendedVec3(position.x, position.y, position.z));
			}

			_owner = GetParent();
		}
	}
}
