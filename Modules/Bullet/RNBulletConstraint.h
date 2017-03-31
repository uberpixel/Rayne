//
//  RNBulletConstraint.h
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BULLETCONSTRAINT_H_
#define __RAYNE_BULLETCONSTRAINT_H_

#include "RNBullet.h"
#include "RNBulletRigidBody.h"

class btTypedConstraint;
namespace RN
{
	class BulletConstraint : public Object
	{
	public:
		BulletConstraint(btTypedConstraint *shape);
			
		BTAPI btTypedConstraint *GetBulletConstraint() const { return _constraint; }
			
	protected:
		BulletConstraint();
		~BulletConstraint() override;
			
		btTypedConstraint *_constraint;
			
		RNDeclareMetaAPI(BulletConstraint, BTAPI)
	};
		
	class BulletFixedConstraint : public BulletConstraint
	{
	public:
		BTAPI BulletFixedConstraint(BulletRigidBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, BulletRigidBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2);
			
		BTAPI static BulletFixedConstraint *WithBodiesAndOffsets(BulletRigidBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, BulletRigidBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2);
			
		RNDeclareMetaAPI(BulletFixedConstraint, BTAPI)
	};
}

#endif /* defined(__RAYNE_BULLETCONSTRAINT_H_) */
