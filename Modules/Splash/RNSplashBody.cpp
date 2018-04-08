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

	}
	void SplashBody::SetAngularVelocity(const Vector3 &velocity)
	{

	}
		
	void SplashBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		
	}
		
		
	void SplashBody::InsertIntoWorld(SplashWorld *world)
	{
		
	}
		
	void SplashBody::RemoveFromWorld(SplashWorld *world)
	{
		
	}

	void SplashBody::SetPositionOffset(RN::Vector3 offset)
	{
		
	}
}