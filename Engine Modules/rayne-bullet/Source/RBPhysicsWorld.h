//
//  RBPhysicsWorld.h
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

#ifndef __RBULLET_PHYSICSWORLD_H__
#define __RBULLET_PHYSICSWORLD_H__

#include <RNVector.h>
#include <RNWorldAttachment.h>
#include <btBulletDynamicsCommon.h>

#include "RBRigidBody.h"

namespace RN
{
	namespace bullet
	{
		class PhysicsWorld : public WorldAttachment
		{
		public:
			PhysicsWorld(const Vector3& gravity=Vector3(0.0f, -9.81f, 0.0f));
			virtual ~PhysicsWorld();
			
			void SetGravity(const Vector3& gravity);
			void AddRigidBody(RigidBody *body);
			void RemoveRigidBody(RigidBody *body);
			
			virtual void StepWorld(float delta);
			virtual void DidAddTransform(Transform *transform);
			virtual void WillRemoveTransform(Transform *transform);
			
			btDynamicsWorld *bulletDynamicsWorld() const { return _dynamicsWorld; }
			
		protected:
			virtual void BuildDynamicsWorld();
				
			btDynamicsWorld *_dynamicsWorld;
			btBroadphaseInterface *_broadphase;
			btCollisionConfiguration *_collisionConfiguration;
			btCollisionDispatcher *_dispatcher;
			btConstraintSolver *_constraintSolver;
			
			std::unordered_set<RigidBody *> _rigidBodies;
			class MetaClass *_rigidBodyClass;
			
			RNDefineMeta(PhysicsWorld, WorldAttachment);
		};
	}
}

#endif /* __RBULLET_PHYSICSWORLD_H__ */
