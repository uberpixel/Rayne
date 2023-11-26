//
//  RNJoltConstraint.h
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLTCONSTRAINT_H_
#define __RAYNE_JOLTCONSTRAINT_H_

#include "RNJolt.h"
#include "RNJoltDynamicBody.h"

/*namespace Jolt
{
	class PxJoint;
	class PxD6JointDrive;
}

namespace RN
{
	class JoltConstraint : public Object
	{
	public:
		JoltConstraint(Jolt::PxJoint *shape);
		JTAPI Jolt::PxJoint *GetJoltConstraint() const { return _constraint; }
		JTAPI void SetMassScale(float scale1, float scale2);
		JTAPI void SetInertiaScale(float scale1, float scale2);
		JTAPI void SetInverseMassScale(float scale1, float scale2);
		JTAPI void SetInverseInertiaScale(float scale1, float scale2);
		
		JTAPI Vector3 GetPositionOffset(size_t bodyIndex);
		JTAPI Quaternion GetRotationOffset(size_t bodyIndex);
		
		JTAPI void SetBreakForce(float force, float torque);
		JTAPI bool IsBroken() const;
			
	protected:
		JoltConstraint();
		~JoltConstraint() override;

		Jolt::PxJoint *_constraint;
			
		RNDeclareMetaAPI(JoltConstraint, JTAPI)
	};
		
	class JoltFixedConstraint : public JoltConstraint
	{
	public:
		JTAPI JoltFixedConstraint(JoltDynamicBody *body1, const Vector3 &offset1, const Quaternion &rotation1, JoltDynamicBody *body2, const Vector3 &offset2, const Quaternion &rotation2);
			
		JTAPI static JoltFixedConstraint *WithBodiesAndOffsets(JoltDynamicBody *body1, const Vector3 &offset1, const Quaternion &rotation1, JoltDynamicBody *body2, const Vector3 &offset2, const Quaternion &rotation2);
			
		RNDeclareMetaAPI(JoltFixedConstraint, JTAPI)
	};

	class JoltRevoluteConstraint : public JoltConstraint
	{
	public:
		JTAPI JoltRevoluteConstraint(JoltDynamicBody *body1, const Vector3 &offset1, const Quaternion &rotation1, JoltDynamicBody *body2, const Vector3 &offset2, const Quaternion &rotation2);
			
		JTAPI static JoltRevoluteConstraint *WithBodiesAndOffsets(JoltDynamicBody *body1, const Vector3 &offset1, const Quaternion &rotation1, JoltDynamicBody *body2, const Vector3 &offset2, const Quaternion &rotation2);
			
		RNDeclareMetaAPI(JoltRevoluteConstraint, JTAPI)
	};

	class JoltD6Drive : public Object
	{
	friend class JoltD6Constraint;
	public:
		JTAPI JoltD6Drive(float stiffness, float damping, float forceLimit, bool isAcceleration);
		JTAPI ~JoltD6Drive();
		
		JTAPI void SetStiffness(float stiffness);
		JTAPI void SetDamping(float damping);
		
	private:
		Jolt::PxD6JointDrive *_drive;
		RNDeclareMetaAPI(JoltD6Drive, JTAPI)
	};

	class JoltD6Constraint : public JoltConstraint
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
		
		JTAPI JoltD6Constraint(JoltDynamicBody *body1, const Vector3 &offset1, const Quaternion &rotation1, JoltDynamicBody *body2, const Vector3 &offset2, const Quaternion &rotation2);
		JTAPI ~JoltD6Constraint();
		
		JTAPI void SetMotion(MotionAxis axis, MotionType type);
		JTAPI void SetLinearLimit(Vector3 lowerLimit, Vector3 upperLimit, float stiffness, float damping);
		JTAPI void SetAngularLimit(Vector3 lowerLimit, Vector3 upperLimit, float stiffness, float damping);
		JTAPI void SetDrive(DriveAxis axis, JoltD6Drive *drive);
		JTAPI void SetDrivePosition(Vector3 position, Quaternion rotation);
		JTAPI void SetDriveVelocity(Vector3 linear, Vector3 angular);
			
		JTAPI static JoltD6Constraint *WithBodiesAndOffsets(JoltDynamicBody *body1, const Vector3 &offset1, const Quaternion &rotation1, JoltDynamicBody *body2, const Vector3 &offset2, const Quaternion &rotation2);
		
	private:
		JoltD6Drive *_drive[6];
			
		RNDeclareMetaAPI(JoltD6Constraint, JTAPI)
	};
}*/

#endif /* defined(__RAYNE_JOLTCONSTRAINT_H_) */
