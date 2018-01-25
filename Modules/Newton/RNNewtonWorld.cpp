//
//  RNNewtonWorld.cpp
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNNewtonWorld.h"
#include "Newton.h"

namespace RN
{
	RNDefineMeta(NewtonWorld, SceneAttachment)

	NewtonWorld *NewtonWorld::_sharedInstance = nullptr;

	NewtonWorld::NewtonWorld(const Vector3 &gravity, bool debug) : _paused(false), _gravity(gravity)
	{
		RN_ASSERT(!_sharedInstance, "There can only be one PhysX instance at a time!");
		_sharedInstance = this;

		// Print the library version.
		RNDebug("Using Newton version %d" << NewtonWorldGetVersion());

		// Create the Newton world.
		_newtonInstance = NewtonCreate();
	}

	NewtonWorld::~NewtonWorld()
	{
		NewtonDestroyAllBodies(_newtonInstance);
		NewtonDestroy(_newtonInstance);

		_sharedInstance = nullptr;
	}

	void NewtonWorld::SetGravity(const Vector3 &gravity)
	{
//		Lock();
		_gravity = gravity;
//		Unlock();
	}

	Vector3 NewtonWorld::GetGravity()
	{
//		Lock();
		return _gravity;
//		Unlock();
	}

	void NewtonWorld::SetPaused(bool paused)
	{
//		Lock();
		_paused = paused;
//		Unlock();
	}

	void NewtonWorld::Update(float delta)
	{
//		Lock();
		bool paused = _paused;
//		Unlock();

		if(paused || delta > 1.0f)
			return;
		
		NewtonUpdate(_newtonInstance, delta);
	}



/*	PhysXContactInfo PhysXWorld::CastRay(const Vector3 &from, const Vector3 &to)
	{
		PhysXContactInfo hit;
		return hit;
	}*/
}
