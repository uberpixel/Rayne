//
//  RNJoltConstraint.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltConstraint.h"
#include "RNJoltInternals.h"
#include "RNJoltWorld.h"
/*
namespace RN
{
	RNDefineMeta(JoltConstraint, Object)
	RNDefineMeta(JoltFixedConstraint, JoltConstraint)
	RNDefineMeta(JoltRevoluteConstraint, JoltConstraint)
	RNDefineMeta(JoltD6Constraint, JoltConstraint)
	RNDefineMeta(JoltD6Drive, Object)
		
	JoltConstraint::JoltConstraint() :
		_constraint(nullptr)
	{}
		
	JoltConstraint::JoltConstraint(Jolt::PxJoint *constraint) :
		_constraint(constraint)
	{}
		
	JoltConstraint::~JoltConstraint()
	{
		_constraint->release();
	}

	void JoltConstraint::SetMassScale(float scale1, float scale2)
	{
		_constraint->setInvMassScale0(1.0f / scale1);
		_constraint->setInvMassScale1(1.0f / scale2);
	}

	void JoltConstraint::SetInertiaScale(float scale1, float scale2)
	{
		_constraint->setInvInertiaScale0(1.0f / scale1);
		_constraint->setInvInertiaScale1(1.0f / scale2);
	}

	void JoltConstraint::SetInverseMassScale(float scale1, float scale2)
	{
		_constraint->setInvMassScale0(scale1);
		_constraint->setInvMassScale1(scale2);
	}

	void JoltConstraint::SetInverseInertiaScale(float scale1, float scale2)
	{
		_constraint->setInvInertiaScale0(scale1);
		_constraint->setInvInertiaScale1(scale2);
	}

	Vector3 JoltConstraint::GetPositionOffset(size_t bodyIndex)
	{
		RN_ASSERT(bodyIndex < 2, "bodyIndex needs to be 0 or 1!");
		const Jolt::PxTransform &transform = _constraint->getLocalPose(static_cast<Jolt::PxJointActorIndex::Enum>(bodyIndex));
		return Vector3(transform.p.x, transform.p.y, transform.p.z);
	}

	Quaternion JoltConstraint::GetRotationOffset(size_t bodyIndex)
	{
		RN_ASSERT(bodyIndex < 2, "bodyIndex needs to be 0 or 1!");
		const Jolt::PxTransform &transform = _constraint->getLocalPose(static_cast<Jolt::PxJointActorIndex::Enum>(bodyIndex));
		return Quaternion(transform.q.x, transform.q.y, transform.q.z, transform.q.w);
	}

	void JoltConstraint::SetBreakForce(float force, float torque)
	{
		_constraint->setBreakForce(force, torque);
	}

	bool JoltConstraint::IsBroken() const
	{
		return (_constraint->getConstraintFlags() & Jolt::PxConstraintFlag::eBROKEN);
	}
		
	JoltFixedConstraint::JoltFixedConstraint(JoltDynamicBody *body1, const Vector3 &offset1, const Quaternion &rotation1, JoltDynamicBody *body2, const Vector3 &offset2, const Quaternion &rotation2)
	{
		Jolt::PxFixedJoint *joint = Jolt::PxFixedJointCreate(*JoltWorld::GetSharedInstance()->GetJoltInstance(),
			body1?body1->GetJoltActor():nullptr, Jolt::PxTransform(Jolt::PxVec3(offset1.x, offset1.y, offset1.z), Jolt::PxQuat(rotation1.x, rotation1.y, rotation1.z, rotation1.w)),
			body2?body2->GetJoltActor():nullptr, Jolt::PxTransform(Jolt::PxVec3(offset2.x, offset2.y, offset2.z), Jolt::PxQuat(rotation2.x, rotation2.y, rotation2.z, rotation2.w)));
		
//		joint->setConstraintFlag(Jolt::PxConstraintFlag::Enum::ePROJECTION, true);
		
		_constraint = joint;
		RN_ASSERT(_constraint, "Probably missconfigured constraint");
	}
		
	JoltFixedConstraint *JoltFixedConstraint::WithBodiesAndOffsets(JoltDynamicBody *body1, const Vector3 &offset1, const Quaternion &rotation1, JoltDynamicBody *body2, const Vector3 &offset2, const Quaternion &rotation2)
	{
		JoltFixedConstraint *constraint = new JoltFixedConstraint(body1, offset1, rotation1, body2, offset2, rotation2);
		return constraint->Autorelease();
	}

	
	JoltRevoluteConstraint::JoltRevoluteConstraint(JoltDynamicBody *body1, const Vector3 &offset1, const Quaternion &rotation1, JoltDynamicBody *body2, const Vector3 &offset2, const Quaternion &rotation2)
	{
		Jolt::PxRevoluteJoint *joint = Jolt::PxRevoluteJointCreate(*JoltWorld::GetSharedInstance()->GetJoltInstance(),
			body1->GetJoltActor(), Jolt::PxTransform(Jolt::PxVec3(offset1.x, offset1.y, offset1.z), Jolt::PxQuat(rotation1.x, rotation1.y, rotation1.z, rotation1.w)),
			body2->GetJoltActor(), Jolt::PxTransform(Jolt::PxVec3(offset2.x, offset2.y, offset2.z), Jolt::PxQuat(rotation2.x, rotation2.y, rotation2.z, rotation2.w)));
		
		_constraint = joint;
		RN_ASSERT(_constraint, "Probably missconfigured constraint");
	}
		
	JoltRevoluteConstraint *JoltRevoluteConstraint::WithBodiesAndOffsets(JoltDynamicBody *body1, const Vector3 &offset1, const Quaternion &rotation1, JoltDynamicBody *body2, const Vector3 &offset2, const Quaternion &rotation2)
	{
		JoltRevoluteConstraint *constraint = new JoltRevoluteConstraint(body1, offset1, rotation1, body2, offset2, rotation2);
		return constraint->Autorelease();
	}


	JoltD6Constraint::JoltD6Constraint(JoltDynamicBody *body1, const Vector3 &offset1, const Quaternion &rotation1, JoltDynamicBody *body2, const Vector3 &offset2, const Quaternion &rotation2)
	{
		Jolt::PxD6Joint *joint = Jolt::PxD6JointCreate(*JoltWorld::GetSharedInstance()->GetJoltInstance(),
			body1->GetJoltActor(), Jolt::PxTransform(Jolt::PxVec3(offset1.x, offset1.y, offset1.z), Jolt::PxQuat(rotation1.x, rotation1.y, rotation1.z, rotation1.w)),
			body2->GetJoltActor(), Jolt::PxTransform(Jolt::PxVec3(offset2.x, offset2.y, offset2.z), Jolt::PxQuat(rotation2.x, rotation2.y, rotation2.z, rotation2.w)));
		
		for(int i = 0; i < 6; i++)
		{
			_drive[i] = nullptr;
		}
		
//		joint->setConstraintFlag(Jolt::PxConstraintFlag::Enum::ePROJECTION, true);
		
		_constraint = joint;
		RN_ASSERT(_constraint, "Probably missconfigured constraint");
	}

	JoltD6Constraint::~JoltD6Constraint()
	{
		for(int i = 0; i < 6; i++)
		{
			SafeRelease(_drive[i]);
		}
	}

	void JoltD6Constraint::SetMotion(MotionAxis axis, MotionType type)
	{
		Jolt::PxD6Axis::Enum pxAxis = static_cast<Jolt::PxD6Axis::Enum>(axis);
		Jolt::PxD6Motion::Enum pxMotion = static_cast<Jolt::PxD6Motion::Enum>(type);
		Jolt::PxD6Joint *joint = static_cast<Jolt::PxD6Joint *>(_constraint);
		joint->setMotion(pxAxis, pxMotion);
	}

	void JoltD6Constraint::SetLinearLimit(Vector3 lowerLimit, Vector3 upperLimit, float stiffness, float damping)
	{
		Jolt::PxD6Joint *joint = static_cast<Jolt::PxD6Joint *>(_constraint);
		joint->setLinearLimit(Jolt::PxD6Axis::Enum::eX, Jolt::PxJointLinearLimitPair(lowerLimit.x, upperLimit.x, Jolt::PxSpring(stiffness, damping)));
		joint->setLinearLimit(Jolt::PxD6Axis::Enum::eY, Jolt::PxJointLinearLimitPair(lowerLimit.y, upperLimit.y, Jolt::PxSpring(stiffness, damping)));
		joint->setLinearLimit(Jolt::PxD6Axis::Enum::eZ, Jolt::PxJointLinearLimitPair(lowerLimit.z, upperLimit.z, Jolt::PxSpring(stiffness, damping)));
	}

	void JoltD6Constraint::SetAngularLimit(Vector3 lowerLimit, Vector3 upperLimit, float stiffness, float damping)
	{
		Jolt::PxD6Joint *joint = static_cast<Jolt::PxD6Joint *>(_constraint);
		joint->setTwistLimit(Jolt::PxJointAngularLimitPair(lowerLimit.x, upperLimit.x, Jolt::PxSpring(stiffness, damping)));
		joint->setPyramidSwingLimit(Jolt::PxJointLimitPyramid(lowerLimit.y, upperLimit.y, lowerLimit.z, upperLimit.z, Jolt::PxSpring(stiffness, damping)));
	}
		
	JoltD6Constraint *JoltD6Constraint::WithBodiesAndOffsets(JoltDynamicBody *body1, const Vector3 &offset1, const Quaternion &rotation1, JoltDynamicBody *body2, const Vector3 &offset2, const Quaternion &rotation2)
	{
		JoltD6Constraint *constraint = new JoltD6Constraint(body1, offset1, rotation1, body2, offset2, rotation2);
		return constraint->Autorelease();
	}

	void JoltD6Constraint::SetDrive(DriveAxis axis, JoltD6Drive *drive)
	{
		SafeRelease(_drive[static_cast<int>(axis)]);
		
		Jolt::PxD6Joint *d6Joint = static_cast<Jolt::PxD6Joint*>(_constraint);
		Jolt::PxD6JointDrive *jointdrive = drive?drive->_drive:nullptr;
		
		bool isValid = jointdrive->isValid();
		if(!isValid)
		{
			RNDebug("Invalid Jolt D6 constraint drive configuration.");
		}
		
		d6Joint->setDrive(static_cast<Jolt::PxD6Drive::Enum>(axis), *jointdrive);
		_drive[static_cast<int>(axis)] = SafeRetain(drive);
	}

	void JoltD6Constraint::SetDrivePosition(Vector3 position, Quaternion rotation)
	{
		Jolt::PxD6Joint *d6Joint = static_cast<Jolt::PxD6Joint*>(_constraint);
		d6Joint->setDrivePosition(Jolt::PxTransform(Jolt::PxVec3(position.x, position.y, position.z), Jolt::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
	}

	void JoltD6Constraint::SetDriveVelocity(Vector3 linear, Vector3 angular)
	{
		Jolt::PxD6Joint *d6Joint = static_cast<Jolt::PxD6Joint*>(_constraint);
		d6Joint->setDriveVelocity(Jolt::PxVec3(linear.x, linear.y, linear.z), Jolt::PxVec3(angular.x, angular.y, angular.z));
	}

	JoltD6Drive::JoltD6Drive(float stiffness, float damping, float forceLimit, bool isAcceleration)
	{
		_drive = new Jolt::PxD6JointDrive(stiffness, damping, forceLimit, isAcceleration);
	}

	JoltD6Drive::~JoltD6Drive()
	{
		delete _drive;
	}
	
	void JoltD6Drive::SetStiffness(float stiffness)
	{
		_drive->stiffness = stiffness;
	}

	void JoltD6Drive::SetDamping(float damping)
	{
		_drive->damping = damping;
	}
}*/
