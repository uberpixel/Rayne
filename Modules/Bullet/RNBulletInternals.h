//
//  RNBulletInternals.h
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BULLETINTERNALS_H_
#define __RAYNE_BULLETINTERNALS_H_

#include "RNBullet.h"
#include "btBulletDynamicsCommon.h"

namespace RN
{
	class BulletRigidBodyMotionState : public btMotionState
	{
	public:
		void SetSceneNode(SceneNode *node);
		void SetPositionOffset(Vector3 offset);

		void getWorldTransform(btTransform &worldTrans) const override;
		void setWorldTransform(const btTransform &worldTrans) override;

	private:
		SceneNode *_node;
		Vector3 _offset;
	};
}

#endif /* defined(__RAYNE_BULLETINTERNALS_H_) */
