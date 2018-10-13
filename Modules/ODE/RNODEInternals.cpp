//
//  RNODEInternals.cpp
//  Rayne-ODE
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNODEInternals.h"

namespace RN
{
/*	void BulletRigidBodyMotionState::SetSceneNode(SceneNodeAttachment *attachment)
	{
		_attachment = attachment;
	}

	void BulletRigidBodyMotionState::SetPositionOffset(Vector3 offset)
	{
		_offset = offset;
	}

	void BulletRigidBodyMotionState::getWorldTransform(btTransform &worldTrans) const
	{
		if(!_attachment)
			return;

		Quaternion rotation = std::move(_attachment->GetWorldRotation());
		Vector3 position = std::move(_attachment->GetWorldPosition() - rotation.GetRotatedVector(_offset));

		worldTrans.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
		worldTrans.setOrigin(btVector3(position.x, position.y, position.z));
	}

	void BulletRigidBodyMotionState::setWorldTransform(const btTransform &worldTrans)
	{
		if(!_attachment)
			return;

		btQuaternion rotation = worldTrans.getRotation();
		btVector3 position = worldTrans.getOrigin();

		_attachment->SetWorldRotation(Quaternion(rotation.x(), rotation.y(), rotation.z(), rotation.w()));
		_attachment->SetWorldPosition(Vector3(position.x(), position.y(), position.z()) + _attachment->GetWorldRotation().GetRotatedVector(_offset));
	}*/
}
