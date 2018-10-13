//
//  RNBulletKinematicController.cpp
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBulletKinematicController.h"
#include "RNBulletWorld.h"
#include "RNBulletMaterial.h"

#include "btBulletDynamicsCommon.h"
#include "BulletDynamics/Character/btKinematicCharacterController.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

namespace RN
{
	RNDefineMeta(BulletKinematicController, BulletCollisionObject)
		
		BulletKinematicController::BulletKinematicController(BulletShape *shape, float stepHeight, BulletShape *ghostShape) :
		_shape(shape->Retain()), _ghostShape(ghostShape?ghostShape:shape)
	{
		_ghostShape->Retain();
		_ghost = new btPairCachingGhostObject();
		_ghost->setCollisionShape(_ghostShape->GetBulletShape());
		_ghost->setCollisionFlags(_ghost->getCollisionFlags() | btCollisionObject::CF_CHARACTER_OBJECT);
		_ghost->setUserPointer(this);
			
		_controller = new btKinematicCharacterController(_ghost, static_cast<btConvexShape *>(_shape->GetBulletShape()), stepHeight);
		_controller->warp(btVector3(offset.x, offset.y, offset.z));
		_controller->setJumpSpeed(5.0f);
		_controller->setGravity(btVector3(0.0f, -9.81f, 0.0f));
	}
		
	BulletKinematicController::~BulletKinematicController()
	{
		delete _ghost;
		delete _controller;
		_ghostShape->Release();
		_shape->Release();
	}
		
		
	void BulletKinematicController::SetWalkDirection(const Vector3 &direction)
	{
		_controller->setWalkDirection(btVector3(direction.x, direction.y, direction.z));
	}
	void BulletKinematicController::SetFallSpeed(float speed)
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
	}
		
	void BulletKinematicController::Update(float delta)
	{
		if(delta < k::EpsilonFloat)
		{
			return;
		}
			
		auto bulletWorld = GetOwner()->GetBulletDynamicsWorld();
		_controller->updateAction(bulletWorld, delta);

		btTransform transform = _ghost->getWorldTransform();
		btVector3 &position = transform.getOrigin();
			
		GetParent()->SetWorldPosition(Vector3(position.x(), position.y(), position.z()) + offset);
	}
		
/*	void BulletKinematicController::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		BulletCollisionObject::DidUpdate(changeSet);
			
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			Vector3 position = GetWorldPosition() - offset;
			_controller->warp(btVector3(position.x, position.y, position.z));
		}
	}*/
	void BulletKinematicController::UpdateFromMaterial(BulletMaterial *material)
	{
		_ghost->setFriction(material->GetFriction());
		_ghost->setRestitution(material->GetRestitution());
	}
		
		
	void BulletKinematicController::InsertIntoWorld(BulletWorld *world)
	{
		BulletCollisionObject::InsertIntoWorld(world);
			
		{
			Vector3 position = GetParent()->GetWorldPosition() - offset;
			_controller->warp(btVector3(position.x, position.y, position.z));
		}
			
		auto bulletWorld = world->GetBulletDynamicsWorld();
			
		bulletWorld->addCollisionObject(_ghost, GetCollisionFilter(), GetCollisionFilterMask());
//		bulletWorld->addAction(_controller);
	}
		
	void BulletKinematicController::RemoveFromWorld(BulletWorld *world)
	{
		BulletCollisionObject::RemoveFromWorld(world);
			
		auto bulletWorld = world->GetBulletDynamicsWorld();
			
		bulletWorld->removeCollisionObject(_ghost);
//		bulletWorld->removeAction(_controller);
	}
}
