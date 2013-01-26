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
#include "RNSpinLock.h"

namespace RN
{
	class PhysicsPipeline;
	class RigidBodyEntity : public Entity, public btMotionState
	{
	friend class PhysicsPipeline;
	public:
		typedef enum 
		{
			ShapeBox,
			ShapeSphere,
			ShapeMesh
		} Shape;
		
		RigidBodyEntity(Shape shape=ShapeBox);
		virtual ~RigidBodyEntity();
		
		virtual void Update(float delta);
		virtual void PostUpdate();
		
		void SetMass(float mass);
		void SetSize(const Vector3& size);
		
		virtual void SetPosition(const Vector3& pos);
		virtual void SetRotation(const Quaternion& rot);
		
	protected:
		void UpdateRigidBody(btDynamicsWorld *world);
		
		virtual void getWorldTransform(btTransform& worldTrans) const;
		virtual void setWorldTransform(const btTransform& worldTrans);
		
	private:
		enum
		{
			MassChange = (1 << 0),
			SizeChange = (1 << 1),
			PositionChange = (1 << 2),
			RotationChange = (1 << 3)
		};
		
		btCollisionShape *GenerateMeshShape();
		
		SpinLock _transformLock;
		Transform _cachedTransform;
		std::mutex _mutex;
		
		float _mass;
		Vector3 _size;
		uint32 _changes;
		
		Shape _shapeType;
		
		btCollisionShape *_shape;
		btTriangleMesh *_triangleMesh;
		btRigidBody *_rigidbody;
	};
}

#endif /* __RAYNE_RIGIDBODYENTITY_H__ */
