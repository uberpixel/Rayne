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
}
