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

	SplashWorld *SplashWorld::_sharedInstance = nullptr;

	SplashWorld::SplashWorld(const Vector3 &gravity) : _stepsPerFrame(1), _paused(false)
	{
		RN_ASSERT(!_sharedInstance, "There can only be one SplashWorld at a time!");

		_sharedInstance = this;
	}

	SplashWorld::~SplashWorld()
	{
		_sharedInstance = nullptr;
	}

	void SplashWorld::SetStepsPerFrame(uint16 stepCount)
	{
		_stepsPerFrame = stepCount;
	}

	void SplashWorld::SetPaused(bool paused)
	{
		_paused = paused;
	}

	void SplashWorld::Update(float delta)
	{
		if(_paused)
			return;

		float stepSize = delta / static_cast<float>(_stepsPerFrame);
		for(uint16 i = 0; i < _stepsPerFrame; i++)
		{
			StepSimulation(stepSize);
		}
	}

	void SplashWorld::StepSimulation(float delta)
	{
		for(SplashBody *body : _bodies)
		{
			body->CalculateVelocities(delta);
			body->PrepareCollision(delta);
		}

		for(auto iter = _bodies.begin(); iter != _bodies.end(); ++iter)
		{
			SplashBody *body = *iter;
			auto iter2 = iter;
			for(++iter2; iter2 != _bodies.end(); ++iter2)
			{
				SplashBody *other = *iter2;
				body->Collide(other, delta);
			}
		}

		for(SplashBody *body : _bodies)
		{
			body->Move(delta);
		}
	}

	void SplashWorld::InsertBody(SplashBody *attachment)
	{
		//TODO: Add lock!?
		auto iterator = _bodies.find(attachment);
		if(iterator == _bodies.end())
		{
			_bodies.insert(attachment);
		}
	}

	void SplashWorld::RemoveBody(SplashBody *attachment)
	{
		//TODO: Add lock!?
		auto iterator = _bodies.find(attachment);
		if(iterator != _bodies.end())
		{
			_bodies.erase(attachment);
		}
	}
}
