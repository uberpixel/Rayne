//
//  RNSplashWorld.cpp
//  Rayne-Splash
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSplashWorld.h"
#include "RNSplashBody.h"

namespace RN
{
	RNDefineMeta(SplashWorld, SceneAttachment)

	SplashWorld::SplashWorld(const Vector3 &gravity) : _maxSteps(50), _stepSize(1.0 / 120.0), _paused(false)
	{

	}

	SplashWorld::~SplashWorld()
	{

	}

	void SplashWorld::SetStepSize(double stepsize, int maxsteps)
	{
		_stepSize = stepsize;
		_maxSteps = maxsteps;
	}

	void SplashWorld::SetPaused(bool paused)
	{
		_paused = paused;
	}

	void SplashWorld::Update(float delta)
	{
		if(_paused)
			return;
	}


	void SplashWorld::InsertCollisionObject(SplashBody *attachment)
	{
		//TODO: Add lock!?
		auto iterator = _collisionObjects.find(attachment);
		if(iterator == _collisionObjects.end())
		{
//			attachment->InsertIntoWorld(this);
			_collisionObjects.insert(attachment);
		}
	}

	void SplashWorld::RemoveCollisionObject(SplashBody *attachment)
	{
		//TODO: Add lock!?
		auto iterator = _collisionObjects.find(attachment);
		if(iterator != _collisionObjects.end())
		{
//			attachment->RemoveFromWorld(this);
			_collisionObjects.erase(attachment);
		}
	}
}
