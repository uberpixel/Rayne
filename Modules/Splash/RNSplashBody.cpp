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
		
	SplashBody::SplashBody(SplashShape *shape, float mass) :
		_shape(shape->Retain()), _mass(mass)
	{
		
	}
		
	SplashBody::~SplashBody()
	{
		SplashWorld::GetSharedInstance()->RemoveBody(this);
		_shape->Release();
	}
	
		
	SplashBody *SplashBody::WithShape(SplashShape *shape, float mass)
	{
		SplashBody *body = new SplashBody(shape, mass);
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

	void SplashBody::AddForce(const RN::Vector3 force)
	{
		if(_mass > 0)
		{
			_linearAcceleration += force / _mass;
		}
	}

	void SplashBody::CalculateVelocities(float delta)
	{
		_linearVelocity += _linearAcceleration;
		_linearAcceleration = Vector3();
	}

	void SplashBody::PrepareCollision(float delta)
	{
		SafeRelease(_transformedShape);
		_transformedShape = _shape->GetTransformedCopy(GetParent()->GetWorldTransform());
		SafeRetain(_transformedShape);
	}

	void SplashBody::Collide(SplashBody *other, float delta)
	{
		if(!_transformedShape || !other->_transformedShape) return;

		RN::Vector3 closestDistance = _transformedShape->GetClosestDistanceVector(other->_transformedShape);

		if(closestDistance.GetDotProduct(_linearVelocity) > 0.0f)
		{
			
		}
	}

	void SplashBody::Move(float delta)
	{
		SetWorldPosition(GetWorldPosition() + _linearVelocity * delta);
	}
}