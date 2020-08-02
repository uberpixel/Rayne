//
//  RNPhysXDynamicBody.h
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSXDYNAMICBODY_H_
#define __RAYNE_PHYSXDYNAMICBODY_H_

#include "RNPhysXCollisionObject.h"

namespace physx
{
	class PxRigidDynamic;
}

namespace RN
{
	class PhysXShape;
	class PhysXDynamicBody : public PhysXCollisionObject
	{
	public:
		enum LockAxis
		{
			LockAxisLinearX = (1 << 0),
			LockAxisLinearY = (1 << 1),
			LockAxisLinearZ = (1 << 2),
			LockAxisAngularX = (1 << 3),
			LockAxisAngularY = (1 << 4),
			LockAxisAngularZ = (1 << 5)
		};
		
		PXAPI PhysXDynamicBody(PhysXShape *shape, float mass);
		PXAPI ~PhysXDynamicBody() override;
			
		PXAPI static PhysXDynamicBody *WithShape(PhysXShape *shape, float mass);

		PXAPI void UpdatePosition() override;

		PXAPI void SetCollisionFilter(uint32 group, uint32 mask) override;
		PXAPI void SetCollisionFilterID(uint32 id, uint32 ignoreid) override;
			
		PXAPI void SetMass(float mass);
		PXAPI void SetLinearVelocity(const Vector3 &velocity);
		PXAPI void SetAngularVelocity(const Vector3 &velocity);
		PXAPI void SetDamping(float linear, float angular);
		PXAPI void SetMaxAngularVelocity(float max);
		PXAPI void SetMaxDepenetrationVelocity(float max);
		PXAPI void SetEnableCCD(bool enable);
		PXAPI void SetEnableGravity(bool enable);
		PXAPI void SetEnableKinematic(bool enable);
		PXAPI void LockMovement(uint32 lockFlags);
		PXAPI void SetSolverIterationCount(uint32 positionIterations, uint32 velocityIterations);

		PXAPI void SetKinematicTarget(const Vector3 &position, const Quaternion &rotation);
		PXAPI void AccelerateToTarget(const Vector3 &position, const Quaternion &rotation, float delta);

		PXAPI void ApplyForce(const Vector3 &force);
//		PXAPI void ApplyForce(const Vector3 &force, const Vector3 &origin);
		PXAPI void ClearForces();
			
		PXAPI void ApplyTorque(const Vector3 &torque);
		PXAPI void ApplyTorqueImpulse(const Vector3 &torque);
		PXAPI void ApplyImpulse(const Vector3 &impulse);
//		PXAPI void ApplyImpulse(const Vector3 &impulse, const Vector3 &origin);
			
		PXAPI float GetMass() const;
		
		PXAPI Vector3 GetLinearVelocity() const;
		PXAPI Vector3 GetAngularVelocity() const;
		
		PXAPI void SetEnableSleeping(bool enable);
		PXAPI bool GetIsSleeping() const;
		
		PXAPI bool GetIsKinematic() const;

		PXAPI bool SweepTest(std::vector<PhysXContactInfo> &contactInfo, const Vector3 &direction, const Vector3 &offsetPosition = Vector3(), const Quaternion &offsetRotation = Quaternion(), float inflation = 0.0f) const;
		PXAPI Quaternion RotationSweepTest(std::vector<PhysXContactInfo> &contactInfo, const Quaternion &targetRoation, float stepSize, float sweepSize, const Vector3 &offsetPosition = Vector3(), const Quaternion &offsetRotation = Quaternion()) const;

		PXAPI physx::PxRigidDynamic *GetPhysXActor() const { return _actor; }
		PXAPI PhysXShape *GetShape() const { return _shape; }
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
//		void UpdateFromMaterial(BulletMaterial *material) override;
			
	private:
		PhysXShape *_shape;
		physx::PxRigidDynamic *_actor;
			
		RNDeclareMetaAPI(PhysXDynamicBody, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXDYNAMICBODY_H_) */
