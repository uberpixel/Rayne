//
//  RNBulletConstraint.cpp
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBulletConstraint.h"
#include "btBulletDynamicsCommon.h"

namespace RN
{
	RNDefineMeta(BulletConstraint, Object)
	RNDefineMeta(BulletFixedConstraint, BulletConstraint)
		
		BulletConstraint::BulletConstraint() :
		_constraint(nullptr)
	{}
		
	BulletConstraint::BulletConstraint(btTypedConstraint *constraint) :
		_constraint(constraint)
	{}
		
	BulletConstraint::~BulletConstraint()
	{
		delete _constraint;
	}
		
		
	BulletFixedConstraint::BulletFixedConstraint(BulletRigidBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, BulletRigidBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2)
	{
		_constraint = new btFixedConstraint(*body1->GetBulletRigidBody(), *body2->GetBulletRigidBody(), btTransform(btQuaternion(rotation1.x, rotation1.y, rotation1.z, rotation1.w), btVector3(offset1.x, offset1.y, offset1.z)), btTransform(btQuaternion(rotation2.x, rotation2.y, rotation2.z, rotation2.w), btVector3(offset2.x, offset2.y, offset2.z)));
	}
		
	BulletFixedConstraint *BulletFixedConstraint::WithBodiesAndOffsets(BulletRigidBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, BulletRigidBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2)
	{
		BulletFixedConstraint *constraint = new BulletFixedConstraint(body1, offset1, rotation1, body2, offset2, rotation2);
		return constraint->Autorelease();
	}
}
