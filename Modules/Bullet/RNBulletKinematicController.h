//
//  RNBulletKinematicController.h
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BULLETKINEMATICCONTROLLER_H_
#define __RAYNE_BULLETKINEMATICCONTROLLER_H_

#include "RNBullet.h"
#include "RNBulletCollisionObject.h"
#include "RNBulletShape.h"

class btPairCachingGhostObject;
class btKinematicCharacterController;

namespace RN
{
	class BulletKinematicController : public BulletCollisionObject
	{
	public:
		BTAPI BulletKinematicController(BulletShape *shape, float stepHeight, BulletShape *ghostShape=nullptr);
		BTAPI ~BulletKinematicController() override;
			
		BTAPI void SetWalkDirection(const Vector3 &direction);
		BTAPI void SetFallSpeed(float speed);
		BTAPI void SetJumpSpeed(float speed);
		BTAPI void SetMaxJumpHeight(float maxHeight);
		BTAPI void SetMaxSlope(float maxSlope);
		BTAPI void SetGravity(float gravity);
			
		BTAPI void Update(float delta) override;
			
		BTAPI bool IsOnGround();
		BTAPI void Jump();
			
		BTAPI btCollisionObject *GetBulletCollisionObject() const override;
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		void UpdateFromMaterial(BulletMaterial *material) override;
			
		void InsertIntoWorld(BulletWorld *world) override;
		void RemoveFromWorld(BulletWorld *world) override;
			
		BulletShape *_shape;
		BulletShape *_ghostShape;
			
		btPairCachingGhostObject *_ghost;
		btKinematicCharacterController *_controller;
        
        bool _isMoving;
			
		RNDeclareMetaAPI(BulletKinematicController, BTAPI)
	};
}

#endif /* defined(__RAYNE_BULLETKINEMATICCONTROLLER_H_) */
