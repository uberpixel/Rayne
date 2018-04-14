//
//  RNSplashBody.cpp
//  Rayne-Splash
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSplashBody.h"
#include "RNSplashWorld.h"

namespace RN
{
	RNDefineMeta(SplashBody, SceneNodeAttachment)
		
	SplashBody::SplashBody(SplashShape *shape) :
		_shape(shape->Retain())
	{
		
	}
		
	SplashBody::~SplashBody()
	{
		SplashWorld::GetSharedInstance()->RemoveBody(this);
		_shape->Release();
	}
	
		
	SplashBody *SplashBody::WithShape(SplashShape *shape)
	{
		SplashBody *body = new SplashBody(shape);
		return body->Autorelease();
	}
	
	void SplashBody::SetLinearVelocity(const Vector3 &velocity)
	{
		_linearVelocity = velocity;
	}

	void SplashBody::SetAngularVelocity(const Vector3 &velocity)
	{
		_angularVelocity = velocity;
	}

	void SplashBody::AccelerateToTarget(const Vector3 &targetPosition, const Quaternion &targetRotation, float delta)
	{
		_linearVelocity = targetPosition - GetWorldPosition();
		_linearVelocity /= delta;
	}
		
	void SplashBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		SceneNodeAttachment::DidUpdate(changeSet);

/*		if(changeSet & SceneNode::ChangeSet::Position)
		{
			Vector3 position = GetWorldPosition() - _offset;
			Quaternion rotation = GetWorldRotation();
			_actor->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
		}*/

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			SplashWorld::GetSharedInstance()->InsertBody(this);
		}
	}

	void SplashBody::SetPositionOffset(const Vector3 &offset)
	{
		_offset = offset;
	}

	void SplashBody::Update(float delta)
	{
		
	}

	void SplashBody::CalculateForces(float delta)
	{
		
	}

	void SplashBody::PrepareCollision(float delta)
	{
		
	}

	void SplashBody::Collide(SplashBody *other, float delta)
	{
		
	}

	void SplashBody::Move(float delta)
	{
		SetWorldPosition(GetWorldPosition() + _linearVelocity * delta);
	}
}