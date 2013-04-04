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

#include "RBPhysicsWorld.h"

namespace RN
{
	namespace bullet
	{
		RNDeclareMeta(PhysicsWorld)
		
		PhysicsWorld::PhysicsWorld(const Vector3& gravity)
		{
			_rigidBodyClass = Catalogue::SharedInstance()->ClassWithName("RigidBody");
			
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
		}
		
		
		
		void PhysicsWorld::SetGravity(const Vector3& gravity)
		{
			_dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
		}
		
		void PhysicsWorld::AddRigidBody(RigidBody *body)
		{
			auto iterator = _rigidBodies.find(body);
			if(iterator == _rigidBodies.end())
			{
				_dynamicsWorld->addRigidBody(body->bulletRigidBody());
				_rigidBodies.insert(body);
			}
		}
		
		void PhysicsWorld::RemoveRigidBody(RigidBody *body)
		{
			auto iterator = _rigidBodies.find(body);
			if(iterator != _rigidBodies.end())
			{
				_dynamicsWorld->removeRigidBody(body->bulletRigidBody());
				_rigidBodies.erase(iterator);
			}
		}
		
		
		void PhysicsWorld::BuildDynamicsWorld()
		{
			_broadphase = new btDbvtBroadphase();
			
			_collisionConfiguration = new btDefaultCollisionConfiguration();
			_dispatcher = new btCollisionDispatcher(_collisionConfiguration);
			
			_constraintSolver = new btSequentialImpulseConstraintSolver();
			
			_dynamicsWorld = new btDiscreteDynamicsWorld(_dispatcher, _broadphase, _constraintSolver, _collisionConfiguration);
		}
		
		
		
		void PhysicsWorld::StepWorld(float delta)
		{
			_dynamicsWorld->stepSimulation(delta, 10);
		}
		
		
		
		void PhysicsWorld::DidAddTransform(Transform *transform)
		{
			if(transform->IsKindOfClass(_rigidBodyClass))
			{
				RigidBody *body = static_cast<RigidBody *>(transform);
				AddRigidBody(body);
			}
		}
		
		void PhysicsWorld::WillRemoveTransform(Transform *transform)
		{
			if(transform->IsKindOfClass(_rigidBodyClass))
			{
				RigidBody *body = static_cast<RigidBody *>(transform);
				RemoveRigidBody(body);
			}
		}
	}
}
