//
//  RNBulletMaterial.cpp
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBulletInternals.h"

namespace RN
{
	void BulletRigidBodyMotionState::SetSceneNode(SceneNode *node)
	{
		_node = node;
	}

	void BulletRigidBodyMotionState::SetPositionOffset(Vector3 offset)
	{
		_offset = offset;
	}

	void BulletRigidBodyMotionState::getWorldTransform(btTransform &worldTrans) const
	{
		if(!_node)
			return;

		Quaternion rotation = std::move(_node->GetWorldRotation());
		Vector3 position = std::move(_node->GetWorldPosition() - rotation.GetRotatedVector(_offset));

		worldTrans.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
		worldTrans.setOrigin(btVector3(position.x, position.y, position.z));
	}

	void BulletRigidBodyMotionState::setWorldTransform(const btTransform &worldTrans)
	{
		if(!_node)
			return;

		btQuaternion rotation = worldTrans.getRotation();
		btVector3 position = worldTrans.getOrigin();

		_node->SetWorldRotation(Quaternion(rotation.x(), rotation.y(), rotation.z(), rotation.w()));
		_node->SetWorldPosition(Vector3(position.x(), position.y(), position.z()) + _node->GetWorldRotation().GetRotatedVector(_offset));
	}
}
