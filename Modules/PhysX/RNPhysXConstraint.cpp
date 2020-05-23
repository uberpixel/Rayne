//
//  RNPhysXConstraint.cpp
//  Rayne-PhysX
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXConstraint.h"
#include "PxPhysicsAPI.h"
#include "RNPhysXWorld.h"

namespace RN
{
	RNDefineMeta(PhysXConstraint, Object)
	RNDefineMeta(PhysXFixedConstraint, PhysXConstraint)
	RNDefineMeta(PhysXRevoluteConstraint, PhysXConstraint)
	RNDefineMeta(PhysXD6Constraint, PhysXConstraint)
	RNDefineMeta(PhysXD6Drive, Object)
		
	PhysXConstraint::PhysXConstraint() :
		_constraint(nullptr)
	{}
		
	PhysXConstraint::PhysXConstraint(physx::PxJoint *constraint) :
		_constraint(constraint)
	{}
		
	PhysXConstraint::~PhysXConstraint()
	{
		_constraint->release();
	}

	void PhysXConstraint::SetMassScale(float scale1, float scale2)
	{
		_constraint->setInvMassScale0(1.0f / scale1);
		_constraint->setInvMassScale1(1.0f / scale2);
	}

	void PhysXConstraint::SetInertiaScale(float scale1, float scale2)
	{
		_constraint->setInvInertiaScale0(1.0f / scale1);
		_constraint->setInvInertiaScale1(1.0f / scale2);
	}

	RN::Vector3 PhysXConstraint::GetPositionOffset(size_t bodyIndex)
	{
		RN_ASSERT(bodyIndex < 2, "bodyIndex needs to be 0 or 1!");
		const physx::PxTransform &transform = _constraint->getLocalPose(static_cast<physx::PxJointActorIndex::Enum>(bodyIndex));
		return RN::Vector3(transform.p.x, transform.p.y, transform.p.z);
	}

	RN::Quaternion PhysXConstraint::GetRotationOffset(size_t bodyIndex)
	{
		RN_ASSERT(bodyIndex < 2, "bodyIndex needs to be 0 or 1!");
		const physx::PxTransform &transform = _constraint->getLocalPose(static_cast<physx::PxJointActorIndex::Enum>(bodyIndex));
		return RN::Quaternion(transform.q.x, transform.q.y, transform.q.z, transform.q.w);
	}
		
	PhysXFixedConstraint::PhysXFixedConstraint(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2)
	{
		physx::PxFixedJoint *joint = physx::PxFixedJointCreate(*PhysXWorld::GetSharedInstance()->GetPhysXInstance(),
			body1->GetPhysXActor(), physx::PxTransform(physx::PxVec3(offset1.x, offset1.y, offset1.z), physx::PxQuat(rotation1.x, rotation1.y, rotation1.z, rotation1.w)),
			body2->GetPhysXActor(), physx::PxTransform(physx::PxVec3(offset2.x, offset2.y, offset2.z), physx::PxQuat(rotation2.x, rotation2.y, rotation2.z, rotation2.w)));
		
		_constraint = joint;
		RN_ASSERT(_constraint, "Probably missconfigured constraint");
	}
		
	PhysXFixedConstraint *PhysXFixedConstraint::WithBodiesAndOffsets(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2)
	{
		PhysXFixedConstraint *constraint = new PhysXFixedConstraint(body1, offset1, rotation1, body2, offset2, rotation2);
		return constraint->Autorelease();
	}

	
	PhysXRevoluteConstraint::PhysXRevoluteConstraint(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2)
	{
		physx::PxRevoluteJoint *joint = physx::PxRevoluteJointCreate(*PhysXWorld::GetSharedInstance()->GetPhysXInstance(),
			body1->GetPhysXActor(), physx::PxTransform(physx::PxVec3(offset1.x, offset1.y, offset1.z), physx::PxQuat(rotation1.x, rotation1.y, rotation1.z, rotation1.w)),
			body2->GetPhysXActor(), physx::PxTransform(physx::PxVec3(offset2.x, offset2.y, offset2.z), physx::PxQuat(rotation2.x, rotation2.y, rotation2.z, rotation2.w)));
		
		_constraint = joint;
		RN_ASSERT(_constraint, "Probably missconfigured constraint");
	}
		
	PhysXRevoluteConstraint *PhysXRevoluteConstraint::WithBodiesAndOffsets(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2)
	{
		PhysXRevoluteConstraint *constraint = new PhysXRevoluteConstraint(body1, offset1, rotation1, body2, offset2, rotation2);
		return constraint->Autorelease();
	}


	PhysXD6Constraint::PhysXD6Constraint(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2)
	{
		physx::PxD6Joint *joint = physx::PxD6JointCreate(*PhysXWorld::GetSharedInstance()->GetPhysXInstance(),
			body1->GetPhysXActor(), physx::PxTransform(physx::PxVec3(offset1.x, offset1.y, offset1.z), physx::PxQuat(rotation1.x, rotation1.y, rotation1.z, rotation1.w)),
			body2->GetPhysXActor(), physx::PxTransform(physx::PxVec3(offset2.x, offset2.y, offset2.z), physx::PxQuat(rotation2.x, rotation2.y, rotation2.z, rotation2.w)));
		
		for(int i = 0; i < 6; i++)
		{
			_drive[i] = nullptr;
		}
		
		joint->setConstraintFlag(physx::PxConstraintFlag::Enum::ePROJECTION, true);
		
		_constraint = joint;
		RN_ASSERT(_constraint, "Probably missconfigured constraint");
	}

	PhysXD6Constraint::~PhysXD6Constraint()
	{
		for(int i = 0; i < 6; i++)
		{
			SafeRelease(_drive[i]);
		}
	}

	void PhysXD6Constraint::SetMotion(MotionAxis axis, MotionType type)
	{
		physx::PxD6Axis::Enum pxAxis = static_cast<physx::PxD6Axis::Enum>(axis);
		physx::PxD6Motion::Enum pxMotion = static_cast<physx::PxD6Motion::Enum>(type);
		physx::PxD6Joint *joint = static_cast<physx::PxD6Joint *>(_constraint);
		joint->setMotion(pxAxis, pxMotion);
	}
		
	PhysXD6Constraint *PhysXD6Constraint::WithBodiesAndOffsets(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2)
	{
		PhysXD6Constraint *constraint = new PhysXD6Constraint(body1, offset1, rotation1, body2, offset2, rotation2);
		return constraint->Autorelease();
	}

	void PhysXD6Constraint::SetDrive(DriveAxis axis, PhysXD6Drive *drive)
	{
		SafeRelease(_drive[static_cast<int>(axis)]);
		
		physx::PxD6Joint *d6Joint = static_cast<physx::PxD6Joint*>(_constraint);
		physx::PxD6JointDrive *jointdrive = drive?drive->_drive:nullptr;
		
		bool isValid = jointdrive->isValid();
		if(!isValid)
		{
			RNDebug("Invalid physx D6 constraint drive configuration.");
		}
		
		d6Joint->setDrive(static_cast<physx::PxD6Drive::Enum>(axis), *jointdrive);
		_drive[static_cast<int>(axis)] = SafeRetain(drive);
	}

	void PhysXD6Constraint::SetDrivePosition(RN::Vector3 position, RN::Quaternion rotation)
	{
		physx::PxD6Joint *d6Joint = static_cast<physx::PxD6Joint*>(_constraint);
		d6Joint->setDrivePosition(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
	}

	void PhysXD6Constraint::SetDriveVelocity(RN::Vector3 linear, RN::Vector3 angular)
	{
		physx::PxD6Joint *d6Joint = static_cast<physx::PxD6Joint*>(_constraint);
		d6Joint->setDriveVelocity(physx::PxVec3(linear.x, linear.y, linear.z), physx::PxVec3(angular.x, angular.y, angular.z));
	}

	PhysXD6Drive::PhysXD6Drive(float stiffness, float damping, float forceLimit, bool isAcceleration)
	{
		_drive = new physx::PxD6JointDrive(stiffness, damping, forceLimit, isAcceleration);
	}

	PhysXD6Drive::~PhysXD6Drive()
	{
		delete _drive;
	}
	
	void PhysXD6Drive::SetStiffness(float stiffness)
	{
		_drive->stiffness = stiffness;
	}

	void PhysXD6Drive::SetDamping(float damping)
	{
		_drive->damping = damping;
	}
}
