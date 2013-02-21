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
			ShapeCapsule,
			ShapeMesh
		} Shape;
		
		RigidBodyEntity(Shape shape=ShapeBox);
		RigidBodyEntity(Shape shape, const Vector3& size, float mass);
		virtual ~RigidBodyEntity();
		
		virtual void PostUpdate();
		
		void SetMass(float mass);
		void SetSize(const Vector3& size);
		void SetDamping(float linear, float angular);
		void SetFriction(float friction);
		void SetRestitution(float restitution);
		
		void ApplyForce(const Vector3& force);
		void ApplyForce(const Vector3& force, const Vector3& origin);
		void ClearForces();
		
		void ApplyTorque(const Vector3& torque);
		void ApplyTorqueImpulse(const Vector3& torque);
		
		void ApplyImpulse(const Vector3& impulse);
		void ApplyImpulse(const Vector3& impulse, const Vector3& origin);
		
		virtual void SetPosition(const Vector3& pos);
		virtual void SetRotation(const Quaternion& rot);
		
		float Mass() const { return _mass; }
		float LinearDamping() const { return _linearDamping; }
		float AngularDamping() const { return _angularDamping; }
		float Friction() const { return _friction; }
		float Restitution() const { return _restitution; }
		
	protected:
		void InitializeProperties();
		void CreateRigidBody();
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
			ForceChange = (1 << 5),
			TorqueChange = (1 << 6),
			ClearForcesChange = (1 << 7),
			FrictionChange = (1 << 8),
			RestitutionChange = (1 << 9),
			ImpulseChange = (1 << 10)
		};
		
		btCollisionShape *GenerateMeshShape();
		
		SpinLock _physicsLock;
		uint32 _changes;
		
		float _mass;
		Vector3 _size;
		float _linearDamping;
		float _angularDamping;
		float _friction;
		float _restitution;
		
		Vector3 _centralForce;
		std::vector<std::tuple<Vector3, Vector3>> _forces;
		
		Vector3 _centralImpulse;
		std::vector<std::tuple<Vector3, Vector3>> _impulses;
		
		Vector3 _torque;
		std::vector<Vector3> _torqueImpulses;
		
		Shape _shapeType;
		
		bool _rigidBodyIsInWorld;
		btCollisionShape *_shape;
		btTriangleMesh *_triangleMesh;
		btRigidBody *_rigidbody;
	};
}

#endif /* __RAYNE_RIGIDBODYENTITY_H__ */
