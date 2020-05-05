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
	
		
	PhysXFixedConstraint::PhysXFixedConstraint(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2)
	{
		physx::PxFixedJoint *joint = physx::PxFixedJointCreate(*PhysXWorld::GetSharedInstance()->GetPhysXInstance(),
			body1->GetPhysXActor(), physx::PxTransform(physx::PxVec3(offset1.x, offset1.y, offset1.z), physx::PxQuat(rotation1.x, rotation1.y, rotation1.z, rotation1.w)),
			body2->GetPhysXActor(), physx::PxTransform(physx::PxVec3(offset2.x, offset2.y, offset2.z), physx::PxQuat(rotation2.x, rotation2.y, rotation2.z, rotation2.w)));
		_constraint = joint;
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
		
		//body1->GetPhysXActor()->setSolverIterationCounts(30, 20);
		//body2->GetPhysXActor()->setSolverIterationCounts(30, 20);
		
		_constraint = joint;
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
}
