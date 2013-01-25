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
#include "RNBullet.h"

namespace RN
{
	class RigidBodyEntity : public Entity, public btMotionState
	{
	public:
		enum ShapeType
		{
			Shape_BOX,
			Shape_SPHERE,
			Shape_MESH
		};
		
		RigidBodyEntity();
		
		virtual ~RigidBodyEntity();
		
		virtual void Update(float delta);
		virtual void PostUpdate();
		
		void InitializeRigidBody(btDynamicsWorld *world);
		void DestroyRigidBody(btDynamicsWorld *world);
		
		void SetMass(float mass){_mass = mass;}
		void SetSize(Vector3 size){_size = size;}
		void SetShape(ShapeType shape);
		
		/**
		 *	Motion state events.
		 *	Functions used internally to set and get the bodys transformation.
		 */
		void getWorldTransform(btTransform &worldTrans) const;
		void setWorldTransform(const btTransform &worldTrans);
		
	private:
		btCollisionShape *GenerateMeshShape();
		
		float _mass;
		Vector3 _size;
		
		ShapeType _shapeType;
		btCollisionShape *_shape;
		btTriangleMesh *_triangleMesh;
		btRigidBody *_rigidbody;
		Vector3 _tempPosition;
		Quaternion _tempRotation;
	};
}

#endif /* __RAYNE_RIGIDBODYENTITY_H__ */
