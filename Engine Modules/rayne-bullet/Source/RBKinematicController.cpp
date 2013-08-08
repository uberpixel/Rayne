//
//  RBKinematicController.cpp
//  rayne-bullet
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include "RBKinematicController.h"

namespace RN
{
	namespace bullet
	{
		RNDeclareMeta(KinematicController)
		
		KinematicController::KinematicController(Shape *shape, float stepHeight)
		{
			RN_ASSERT(shape, "There already is a shape!");
			
			_shape = shape->Retain();
			_controller = 0;
			_stepHeight = stepHeight;
		}
		
		KinematicController::~KinematicController()
		{
			if(_controller)
				delete _controller;
		}
		
		
		void KinematicController::SetPosition(const Vector3& position)
		{
			Entity::SetPosition(position);
			bulletCollisionObject();
			
			const Vector3& wPosition = WorldPosition();
			_controller->warp(btVector3(wPosition.x-_offset.x, wPosition.y-_offset.y, wPosition.z-_offset.z));
		}
		
		void KinematicController::SetWorldPosition(const Vector3& position)
		{
			Entity::SetWorldPosition(position);
			bulletCollisionObject();
			
			_controller->warp(btVector3(position.x-_offset.x, position.y-_offset.y, position.z-_offset.z));
		}
		
		
		void KinematicController::SetWalkDirection(const Vector3& direction)
		{
			bulletCollisionObject();
			_controller->setWalkDirection(btVector3(direction.x, direction.y, direction.z));
		}
		
		void KinematicController::SetFallSpeed(float speed)
		{
			bulletCollisionObject();
			_controller->setFallSpeed(speed);
		}
		
		void KinematicController::SetJumpSpeed(float speed)
		{
			bulletCollisionObject();
			_controller->setJumpSpeed(speed);
		}
		
		void KinematicController::SetMaxJumpHeight(float maxHeight)
		{
			bulletCollisionObject();
			_controller->setMaxJumpHeight(maxHeight);
		}
		
		void KinematicController::SetMaxSlope(float maxSlope)
		{
			bulletCollisionObject();
			_controller->setMaxSlope(maxSlope);
		}
		
		
		bool KinematicController::IsOnGround()
		{
			bulletCollisionObject();
			return _controller->canJump();
		}
		
		void KinematicController::Jump()
		{
			bulletCollisionObject();
			_controller->jump();
		}
		
		
		void KinematicController::Update(float delta)
		{
			btPairCachingGhostObject *ghost = bulletCollisionObject<btPairCachingGhostObject>();
			btTransform transform = ghost->getWorldTransform();
			
			btVector3& position = transform.getOrigin();
			
			Entity::SetWorldPosition(Vector3(position.x(), position.y(), position.z())+_offset);
			Entity::Update(delta);
		}
		
		btCollisionObject *KinematicController::CreateCollisionObject()
		{
			const Vector3& position = WorldPosition();
			btPairCachingGhostObject *ghost = new btPairCachingGhostObject();
			
			ghost->setCollisionShape(_shape->bulletShape());
			ghost->setCollisionFlags(ghost->getCollisionFlags() | btCollisionObject::CF_CHARACTER_OBJECT);
			
			_controller = new btKinematicCharacterController(ghost, static_cast<btConvexShape *>(_shape->bulletShape()), _stepHeight);
			_controller->warp(btVector3(position.x-_offset.x, position.y-_offset.y, position.z-_offset.z));
			_controller->setJumpSpeed(9.81);
			_controller->setGravity(9.81);
			
			return ghost;
		}
		
		void KinematicController::InsertIntoWorld(btDynamicsWorld *world)
		{
			btPairCachingGhostObject *ghost = bulletCollisionObject<btPairCachingGhostObject>();
			
			world->addCollisionObject(ghost, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);
			world->addAction(_controller);
		}
		
		void KinematicController::RemoveFromWorld(btDynamicsWorld *world)
		{
			btPairCachingGhostObject *ghost = bulletCollisionObject<btPairCachingGhostObject>();
			
			world->removeCollisionObject(ghost);
			world->removeAction(_controller);
		}
	}
}
