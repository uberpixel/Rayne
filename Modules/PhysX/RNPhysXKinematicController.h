//
//  RNPhysXKinematicController.h
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSXKINEMATICCONTROLLER_H_
#define __RAYNE_PHYSXKINEMATICCONTROLLER_H_

#include "RNPhysX.h"
#include "RNPhysXCollisionObject.h"
#include "RNPhysXShape.h"

namespace physx
{
	class PxController;
}

namespace RN
{
	class PhysXKinematicController : public PhysXCollisionObject
	{
	public:
		PXAPI PhysXKinematicController(float radius, float height, PhysXMaterial *material);
		PXAPI ~PhysXKinematicController() override;
			
		PXAPI void Move(const Vector3 &direction, float delta);
	/*	PXAPI void SetFallSpeed(float speed);
		PXAPI void SetJumpSpeed(float speed);
		PXAPI void SetMaxJumpHeight(float maxHeight);
		PXAPI void SetMaxSlope(float maxSlope);
		PXAPI void SetGravity(float gravity);*/
			
/*		PXAPI bool IsOnGround();
		PXAPI void Jump();
			
		PXAPI btCollisionObject *GetBulletCollisionObject() const override;*/
			
	protected:
		//void DidUpdate(SceneNode::ChangeSet changeSet) override;
			
		void InsertIntoWorld(PhysXWorld *world) override;
		void RemoveFromWorld(PhysXWorld *world) override;
			
		physx::PxController *_controller;
		PhysXMaterial *_material;
			
		RNDeclareMetaAPI(PhysXKinematicController, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXKINEMATICCONTROLLER_H_) */
