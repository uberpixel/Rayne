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
		
		PhysXKinematicController::PhysXKinematicController(float radius, float height, PhysXMaterial *material)
	{
		_material = material->Retain();

		physx::PxCapsuleControllerDesc desc;
		desc.height = height;
		desc.radius = radius;
		desc.position.set(-offset.x, 10.0 - offset.y, -offset.z);
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

		physx::PxControllerCollisionFlags collisionFlags = _controller->move(physx::PxVec3(direction.x, direction.y, direction.z), 0.0f, delta, physx::PxControllerFilters());

		const physx::PxExtendedVec3 &position = _controller->getPosition();

		GetParent()->SetWorldPosition(Vector3(position.x, position.y, position.z) + offset);
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
	}
		
	btCollisionObject *BulletKinematicController::GetBulletCollisionObject() const
	{
		return _ghost;
	}*/
		
/*	void BulletKinematicController::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		BulletCollisionObject::DidUpdate(changeSet);
			
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			Vector3 position = GetWorldPosition() - offset;
			_controller->warp(btVector3(position.x, position.y, position.z));
		}
	}*/
		
		
	void PhysXKinematicController::InsertIntoWorld(PhysXWorld *world)
	{
		PhysXCollisionObject::InsertIntoWorld(world);

		Vector3 position = GetParent()->GetWorldPosition() - offset;
		_controller->setPosition(physx::PxExtendedVec3(position.x, position.y, position.z));
	}
		
	void PhysXKinematicController::RemoveFromWorld(PhysXWorld *world)
	{
		PhysXCollisionObject::RemoveFromWorld(world);
	}
}
