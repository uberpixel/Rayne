//
//  RBCollisionObject.h
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

#ifndef __RBULLET_COLLISIONOBJECT_H__
#define __RBULLET_COLLISIONOBJECT_H__

#include <RNEntity.h>
#include <btBulletDynamicsCommon.h>

#include "RBPhysicsMaterial.h"

namespace RN
{
	namespace bullet
	{
		class PhysicsWorld;
		class CollisionObject : public Entity
		{
		public:
			friend class PhysicsWorld;
			
			void SetMaterial(PhysicsMaterial *material);
			PhysicsMaterial *Material() const { return _material; }
			
			void SetOffset(Vector3 offset);
			
			template<typename T=btCollisionObject>
			T *bulletCollisionObject()
			{
				std::call_once(_objetFlag, [this]{
					_object = CreateCollisionObject();
				});
				
				return static_cast<T *>(_object);
			}
			
			void SetCollisionFilter(short int filter);
			void SetCollisionFilterMask(short int mask);
			
			short int CollisionFilter() const { return _collisionFilter; }
			short int CollisionFilterMask() const { return _collisionFilterMask; }
			
		protected:
			CollisionObject();
			virtual ~CollisionObject();
			
			virtual btCollisionObject *CreateCollisionObject() = 0;
			virtual void ApplyPhysicsMaterial(PhysicsMaterial *material);
			
			virtual void ReinsertIntoWorld();
			virtual void InsertIntoWorld(PhysicsWorld *world);
			virtual void RemoveFromWorld(PhysicsWorld *world);
			
			PhysicsWorld *_world;
			
			PhysicsMaterial *_material;
			btCollisionObject *_object;
			
			short int _collisionFilter;
			short int _collisionFilterMask;
			
			Vector3 _offset;
			
		private:			
			std::once_flag _objetFlag;
			
			RNDefineMeta(CollisionObject, Entity);
		};
	}
}

#endif /* __RBULLET_COLLISIONOBJECT_H__ */
