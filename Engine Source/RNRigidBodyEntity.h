//
//  RNRigidBodyEntity.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RIGIDBODYENTITY_H__
#define __RAYNE_RIGIDBODYENTITY_H__

#include "RNBase.h"
#include "RNEntity.h"
#include <btBulletDynamicsCommon.h>

namespace RN
{
	class RigidBodyEntity : public Entity, public btMotionState
	{
	public:
		RigidBodyEntity();
		
		virtual ~RigidBodyEntity();
		
		virtual void Update(float delta);
		virtual void PostUpdate();
		
		void InitializeRigidBody(btDynamicsWorld *world);
		void UpdateRigidBody();
		
		void SetMass(float mass){_mass = mass;}
		void SetSize(Vector3 size){_size = size;}
		
		/**
		 *	Motion state events.
		 *	Functions used internally to set and get the bodys transformation.
		 */
		void getWorldTransform(btTransform &worldTrans) const;
		void setWorldTransform(const btTransform &worldTrans);
		
	private:
		float _mass;
		Vector3 _size;
		
		btCollisionShape *_shape;
		btRigidBody *_rigidbody;
		Vector3 _tempPosition;
		Quaternion _tempRotation;
	};
}

#endif /* __RAYNE_RIGIDBODYENTITY_H__ */
