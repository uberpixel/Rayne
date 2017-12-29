//
//  RNPhysXRigidBody.h
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSXRIGIDBODY_H_
#define __RAYNE_PHYSXRIGIDBODY_H_

#include "RNPhysXCollisionObject.h"

namespace physx
{
	class PxRigidActor;
}

namespace RN
{
	class PhysXShape;
	class PhysXRigidBody : public PhysXCollisionObject
	{
	public:
		PXAPI PhysXRigidBody(PhysXShape *shape, float mass);
		PXAPI ~PhysXRigidBody() override;
			
		PXAPI static PhysXRigidBody *WithShape(PhysXShape *shape, float mass);
			
/*		PXAPI void SetMass(float mass);
		PXAPI void SetMass(float mass, const Vector3 &inertia);
		PXAPI void SetLinearVelocity(const Vector3 &velocity);
		PXAPI void SetAngularVelocity(const Vector3 &velocity);
		PXAPI void SetCCDMotionThreshold(float threshold);
		PXAPI void SetCCDSweptSphereRadius(float radius);
		PXAPI void SetGravity(const Vector3 &gravity);
		PXAPI void SetDamping(float linear, float angular);
		PXAPI void SetAllowDeactivation(bool canDeactivate);
			
		PXAPI void ApplyForce(const Vector3 &force);
		PXAPI void ApplyForce(const Vector3 &force, const Vector3 &origin);
		PXAPI void ClearForces();
			
		PXAPI void ApplyTorque(const Vector3 &torque);
		PXAPI void ApplyTorqueImpulse(const Vector3 &torque);
		PXAPI void ApplyImpulse(const Vector3 &impulse);
		PXAPI void ApplyImpulse(const Vector3 &impulse, const Vector3 &origin);
			
		PXAPI Vector3 GetLinearVelocity() const;
		PXAPI Vector3 GetAngularVelocity() const;
			
		PXAPI Vector3 GetCenterOfMass() const;
		PXAPI Matrix GetCenterOfMassTransform() const;
			*/
/*		PXAPI btCollisionObject *GetBulletCollisionObject() const override;
		PXAPI btRigidBody *GetBulletRigidBody() { return _rigidBody; }*/

//		PXAPI void SetPositionOffset(RN::Vector3 offset) final;
			
	protected:
/*		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		void UpdateFromMaterial(BulletMaterial *material) override;*/
		
		void InsertIntoWorld(PhysXWorld *world) override;
		void RemoveFromWorld(PhysXWorld *world) override;
			
	private:
		PhysXShape *_shape;
		
		physx::PxRigidActor *_actor;
			
		RNDeclareMetaAPI(PhysXRigidBody, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXRIGIDBODY_H_) */
