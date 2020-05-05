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

	class PhysXD6Constraint : public PhysXConstraint
	{
	public:
		enum MotionAxis
		{
			TranslationX,
			TranslationY,
			TranslationZ,
			RotationX,
			RotationY,
			RotationZ
		};
		
		enum MotionType
		{
			Locked,
			Limited,
			Free
		};
		
		PXAPI PhysXD6Constraint(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2);
		
		PXAPI void SetMotion(MotionAxis axis, MotionType type);
			
		PXAPI static PhysXD6Constraint *WithBodiesAndOffsets(PhysXDynamicBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, PhysXDynamicBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2);
			
		RNDeclareMetaAPI(PhysXD6Constraint, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXCONSTRAINT_H_) */
