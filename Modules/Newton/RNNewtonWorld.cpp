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

	int NewtonWorld::AABBOverlapCallback(const NewtonMaterial* const material, const NewtonBody* const body0, const NewtonBody* const body1, int threadIndex)
	{
		NewtonCollisionObject *object0 = static_cast<NewtonCollisionObject*>(NewtonBodyGetUserData(body0));
		NewtonCollisionObject *object1 = static_cast<NewtonCollisionObject*>(NewtonBodyGetUserData(body1));

		bool filterMask = (object0->GetCollisionFilterGroup() & object1->GetCollisionFilterMask()) && (object1->GetCollisionFilterGroup() & object0->GetCollisionFilterMask());
		bool filterID = (object0->GetCollisionFilterIgnoreID() == 0 && object1->GetCollisionFilterIgnoreID() == 0) || (object0->GetCollisionFilterID() != object1->GetCollisionFilterIgnoreID() && object0->GetCollisionFilterIgnoreID() != object1->GetCollisionFilterID());
		if(filterMask && filterID)
			return 1;

		return 0;
	}

	void NewtonWorld::ProcessCallback(const NewtonJoint* const contact, float timestep, int threadIndex)
	{
		
	}

	NewtonWorld::NewtonWorld(const Vector3 &gravity, bool debug) : _paused(false), _gravity(gravity)
	{
		RN_ASSERT(!_sharedInstance, "There can only be one PhysX instance at a time!");
		_sharedInstance = this;

		// Print the library version.
		RNDebug("Using Newton version %d" << NewtonWorldGetVersion());

		// Create the Newton world.
		_newtonInstance = NewtonCreate();
		NewtonMaterialSetCollisionCallback(_newtonInstance, 0, 0, AABBOverlapCallback, ProcessCallback);
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

	void NewtonWorld::SetSubsteps(uint8 substeps)
	{
		NewtonSetNumberOfSubsteps(_newtonInstance, substeps);
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
