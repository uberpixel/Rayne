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
		
	void SplashBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		
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

	}
}