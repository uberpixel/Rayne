//
//  RBPhysicsWorld.cpp
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

#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include "RBPhysicsWorld.h"

namespace RN
{
	namespace bullet
	{
		RNDeclareMeta(PhysicsWorld)
		
		PhysicsWorld::PhysicsWorld(const Vector3& gravity)
		{
			_collisionObjectClass = Catalogue::GetSharedInstance()->GetClassWithName("RN::bullet::CollisionObject");
			RN_ASSERT(_collisionObjectClass, "");
			
			BuildDynamicsWorld();
			SetGravity(gravity);
		}
		
		PhysicsWorld::~PhysicsWorld()
		{
			delete _dynamicsWorld;
			delete _constraintSolver;
			delete _dispatcher;
			delete _collisionConfiguration;
			delete _broadphase;
			delete _pairCallback;
		}
		
		
		
		void PhysicsWorld::SetGravity(const Vector3& gravity)
		{
			Lock();
			_dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
			Unlock();
		}
		
		void PhysicsWorld::AddCollisionObject(CollisionObject *object)
		{
			auto iterator = _collisionObjects.find(object);
			if(iterator == _collisionObjects.end())
			{
				Lock();
				object->InsertIntoWorld(_dynamicsWorld);
				Unlock();
				
				_collisionObjects.insert(object);
			}
		}
		
		void PhysicsWorld::RemoveCollisionObject(CollisionObject *object)
		{
			auto iterator = _collisionObjects.find(object);
			if(iterator != _collisionObjects.end())
			{
				Lock();
				object->RemoveFromWorld(_dynamicsWorld);
				Unlock();
				
				_collisionObjects.erase(iterator);
			}
		}
		
		Hit PhysicsWorld::RayTrace(const Vector3& from, const Vector3& to)
		{
			btVector3 btRayFrom = btVector3(from.x, from.y, from.z);
			btVector3 btRayTo   = btVector3(to.x, to.y, to.z);
			
			btCollisionWorld::ClosestRayResultCallback rayCallback(btRayFrom, btRayTo);
			
			Lock();
			_dynamicsWorld->rayTest(btRayFrom, btRayTo, rayCallback);
			Unlock();
			
			Hit hit;
			
			if(rayCallback.hasHit())
			{
				CollisionObject *body = reinterpret_cast<CollisionObject *>(rayCallback.m_collisionObject->getUserPointer());
			
				hit.node = body;
				hit.position = Vector3(rayCallback.m_hitPointWorld.x(), rayCallback.m_hitPointWorld.y(), rayCallback.m_hitPointWorld.z());
				hit.normal   = Vector3(rayCallback.m_hitNormalWorld.x(), rayCallback.m_hitNormalWorld.y(), rayCallback.m_hitNormalWorld.z());
			}
			
			return hit;
		}
		
		
		void PhysicsWorld::BuildDynamicsWorld()
		{
			_pairCallback = new btGhostPairCallback();
			
			_broadphase = new btDbvtBroadphase();
			_broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(_pairCallback);
			
			_collisionConfiguration = new btDefaultCollisionConfiguration();
			_dispatcher = new btCollisionDispatcher(_collisionConfiguration);
			
			_constraintSolver = new btSequentialImpulseConstraintSolver();
			
			_dynamicsWorld = new btDiscreteDynamicsWorld(_dispatcher, _broadphase, _constraintSolver, _collisionConfiguration);
		}
		
		
		
		void PhysicsWorld::StepWorld(float delta)
		{
			_dynamicsWorld->stepSimulation(delta, 10);
		}
		
		
		
		void PhysicsWorld::DidAddSceneNode(SceneNode *node)
		{
			if(node->IsKindOfClass(_collisionObjectClass))
			{
				CollisionObject *object = static_cast<CollisionObject *>(node);
				AddCollisionObject(object);
			}
		}
		
		void PhysicsWorld::WillRemoveSceneNode(SceneNode *node)
		{
			if(node->IsKindOfClass(_collisionObjectClass))
			{
				CollisionObject *object = static_cast<CollisionObject *>(node);
				RemoveCollisionObject(object);
			}
		}
	}
}
