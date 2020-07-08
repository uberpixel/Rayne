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
        _shape(shape->Retain()), _ghostShape(ghostShape?ghostShape:shape), _isMoving(false), _isBlocked(false), _isFirstUpdate(true)
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
        _intendedMovement = direction;
        
        _isMoving = false;
        if(direction.GetLength() > k::EpsilonFloat || (!IsOnGround() && std::abs(_controller->getFallSpeed()) > k::EpsilonFloat))
        {
            _isMoving = true;
        }
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

    bool BulletKinematicController::IsBlocked()
    {
        return _isBlocked;
    }
		
	btCollisionObject *BulletKinematicController::GetBulletCollisionObject() const
	{
		return _ghost;
	}
		
	void BulletKinematicController::Update(float delta)
	{
		if(delta < k::EpsilonFloat || (!_isMoving && !_isFirstUpdate))
		{
			return;
		}
        
        _isFirstUpdate = false;
        
		auto bulletWorld = GetOwner()->GetBulletDynamicsWorld();
		_controller->updateAction(bulletWorld, delta);

		btTransform transform = _ghost->getWorldTransform();
		btVector3 &position = transform.getOrigin();
        
        RN::Vector3 newPosition(position.x(), position.y(), position.z());
        newPosition += offset;
        float moveDistance = GetWorldPosition().GetSquaredDistance(newPosition);
        
        _isBlocked = false;
        if(moveDistance < _intendedMovement.GetDotProduct(_intendedMovement) * 0.5f)
        {
            _isBlocked = true;
        }
			
		GetParent()->SetWorldPosition(newPosition);
	}
		
	void BulletKinematicController::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		BulletCollisionObject::DidUpdate(changeSet);
			
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			Vector3 position = GetWorldPosition() - offset;
            btTransform transform = _ghost->getWorldTransform();
            btVector3 &currentPosition = transform.getOrigin();
            
            if(Vector3(currentPosition.x(), currentPosition.y(), currentPosition.z()).GetSquaredDistance(position) > k::EpsilonFloat * 10.0f)
                _controller->warp(btVector3(position.x, position.y, position.z));
		}
	}

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
