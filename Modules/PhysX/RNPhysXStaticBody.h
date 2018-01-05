//
//  RNPhysXStaticBody.h
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSXSTATICBODY_H_
#define __RAYNE_PHYSXSTATICBODY_H_

#include "RNPhysXCollisionObject.h"

namespace physx
{
	class PxRigidStatic;
}

namespace RN
{
	class PhysXShape;
	class PhysXStaticBody : public PhysXCollisionObject
	{
	public:
		PXAPI PhysXStaticBody(PhysXShape *shape);
		PXAPI ~PhysXStaticBody() override;
			
		PXAPI static PhysXStaticBody *WithShape(PhysXShape *shape);

		PXAPI void UpdatePosition() override;

		PXAPI void SetCollisionFilter(uint32 group, uint32 mask) override;
			
		PXAPI physx::PxRigidStatic *GetPhysXActor() const { return _actor; }
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
			
	private:
		PhysXShape *_shape;
		physx::PxRigidStatic *_actor;
			
		bool _didUpdatePosition;

		RNDeclareMetaAPI(PhysXStaticBody, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXSTATICBODY_H_) */
