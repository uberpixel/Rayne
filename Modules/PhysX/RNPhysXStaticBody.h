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

		void Update(float delta);

		void SetCollisionFilter(uint32 group, uint32 mask) override;
			
/*		PXAPI btCollisionObject *GetBulletCollisionObject() const override;
		PXAPI btRigidBody *GetBulletRigidBody() { return _rigidBody; }*/

//		PXAPI void SetPositionOffset(RN::Vector3 offset) final;
			
	protected:
/*		void DidUpdate(SceneNode::ChangeSet changeSet) override;*/
		
		void InsertIntoWorld(PhysXWorld *world) override;
		void RemoveFromWorld(PhysXWorld *world) override;
			
	private:
		PhysXShape *_shape;
		
		physx::PxRigidStatic *_actor;
			
		RNDeclareMetaAPI(PhysXStaticBody, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXSTATICBODY_H_) */
