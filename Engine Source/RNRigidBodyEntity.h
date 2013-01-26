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
		
		virtual void PostUpdate();
		
		void SetMass(float mass);
		void SetSize(const Vector3& size);
		void SetDamping(float linear, float angular);
		
		void ApplyForce(const Vector3& force);
		void ApplyForce(const Vector3& force, const Vector3& origin);
		
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
			RotationChange = (1 << 3),
			DampingChange = (1 << 4),
			ForceChange = (1 << 5)
		};
		
		btCollisionShape *GenerateMeshShape();
		
		Transform _cachedTransform;
		
		SpinLock _physicsLock;
		float _mass;
		Vector3 _size;
		uint32 _changes;
		float _linearDamping;
		float _angularDamping;
		
		Vector3 _centralForce;
		std::vector<std::tuple<Vector3, Vector3>> _forces;
		
		Shape _shapeType;
		
		btCollisionShape *_shape;
		btTriangleMesh *_triangleMesh;
		btRigidBody *_rigidbody;
	};
}

#endif /* __RAYNE_RIGIDBODYENTITY_H__ */
