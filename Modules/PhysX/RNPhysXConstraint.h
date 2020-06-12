//
//  RNPhysXConstraint.h
//  Rayne-PhysX
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSXCONSTRAINT_H_
#define __RAYNE_PHYSXCONSTRAINT_H_

#include "RNPhysX.h"
#include "RNPhysXDynamicBody.h"

namespace physx
{
	class PxJoint;
	class PxD6JointDrive;
}

namespace RN
{
	class PhysXConstraint : public Object
	{
	public:
		PhysXConstraint(physx::PxJoint *shape);
		PXAPI physx::PxJoint *GetPhysXConstraint() const { return _constraint; }
		PXAPI void SetMassScale(float scale1, float scale2);
		PXAPI void SetInertiaScale(float scale1, float scale2);
		
		PXAPI RN::Vector3 GetPositionOffset(size_t bodyIndex);
		PXAPI RN::Quaternion GetRotationOffset(size_t bodyIndex);
			
	protected:
		PhysXConstraint();
		~PhysXConstraint() override;

		physx::PxJoint *_constraint;
			
		RNDeclareMetaAPI(PhysXConstraint, PXAPI)
	};
		
	class PhysXFixedConstraint : public PhysXConstraint
	{
	public:
		PXAPI PhysXFixedConstraint(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2);
			
		PXAPI static PhysXFixedConstraint *WithBodiesAndOffsets(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2);
			
		RNDeclareMetaAPI(PhysXFixedConstraint, PXAPI)
	};

	class PhysXRevoluteConstraint : public PhysXConstraint
	{
	public:
		PXAPI PhysXRevoluteConstraint(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2);
			
		PXAPI static PhysXRevoluteConstraint *WithBodiesAndOffsets(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2);
			
		RNDeclareMetaAPI(PhysXRevoluteConstraint, PXAPI)
	};

	class PhysXD6Drive : public Object
	{
	friend class PhysXD6Constraint;
	public:
		PXAPI PhysXD6Drive(float stiffness, float damping, float forceLimit, bool isAcceleration);
		PXAPI ~PhysXD6Drive();
		
		PXAPI void SetStiffness(float stiffness);
		PXAPI void SetDamping(float damping);
		
	private:
		physx::PxD6JointDrive *_drive;
		RNDeclareMetaAPI(PhysXD6Drive, PXAPI)
	};

	class PhysXD6Constraint : public PhysXConstraint
	{
	public:
		enum MotionAxis
		{
			motionLinearX,
			motionLinearY,
			motionLinearZ,
			motionAngularX,
			motionAngularY,
			motionAngularZ
		};
		
		enum DriveAxis
		{
			DriveLinearX,
			DriveLinearY,
			DriveLinearZ,
			DriveTwist,
			DriveSpin,
			DriveSlerp
		};
		
		enum MotionType
		{
			Locked,
			Limited,
			Free
		};
		
		PXAPI PhysXD6Constraint(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2);
		PXAPI ~PhysXD6Constraint();
		
		PXAPI void SetMotion(MotionAxis axis, MotionType type);
		PXAPI void SetDrive(DriveAxis axis, PhysXD6Drive *drive);
		PXAPI void SetDrivePosition(RN::Vector3 position, RN::Quaternion rotation);
		PXAPI void SetDriveVelocity(RN::Vector3 linear, RN::Vector3 angular);
			
		PXAPI static PhysXD6Constraint *WithBodiesAndOffsets(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2);
		
	private:
		PhysXD6Drive *_drive[6];
			
		RNDeclareMetaAPI(PhysXD6Constraint, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXCONSTRAINT_H_) */
