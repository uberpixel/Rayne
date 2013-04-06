//
//  RBKinematicController.h
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

#ifndef __RBULLET_KINEMATICCONTROLLER_H__
#define __RBULLET_KINEMATICCONTROLLER_H__

#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include "RBCollisionObject.h"
#include "RBShape.h"

namespace RN
{
	namespace bullet
	{
		class KinematicController : public CollisionObject
		{
		public:
			KinematicController(Shape *shape, float stepHeight);
			virtual ~KinematicController();
			
			virtual void SetPosition(const Vector3& position);
			virtual void SetWorldPosition(const Vector3& position);
			
			void SetWalkDirection(const Vector3& direction);
			void SetFallSpeed(float speed);
			void SetJumpSpeed(float speed);
			void SetMaxJumpHeight(float maxHeight);
			void SetMaxSlope(float maxSlope);
			
			virtual void Update(float delta);
			
			bool IsOnGround();
			void Jump();
			
			Shape *CollisionShape() const { return _shape; }
			
		protected:
			virtual btCollisionObject *CreateCollisionObject();
			virtual void InsertIntoWorld(btDynamicsWorld *world);
			virtual void RemoveFromWorld(btDynamicsWorld *world);
			
			Shape *_shape;
			btKinematicCharacterController *_controller;
			float _stepHeight;
			
			RNDefineConstructorlessMeta(KinematicController, CollisionObject)
		};
	}
}

#endif /* __RBULLET_KINEMATICCONTROLLER_H__ */
