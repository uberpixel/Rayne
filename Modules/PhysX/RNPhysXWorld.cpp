//
//  RNPhysXWorld.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXWorld.h"
#include "PxPhysicsAPI.h"

namespace RN
{
	RNDefineMeta(PhysXWorld, SceneAttachment)

	physx::PxDefaultErrorCallback gDefaultErrorCallback;
	physx::PxDefaultAllocator gDefaultAllocatorCallback;

	PhysXWorld *PhysXWorld::_sharedInstance = nullptr;

	PhysXWorld::PhysXWorld(const Vector3 &gravity, bool debug) : _pvd(nullptr), _remainingTime(0.0), _stepSize(1.0 / 90.0), _paused(false)
	{
		RN_ASSERT(!_sharedInstance, "There can only be one PhysX instance at a time!");
		_sharedInstance = this;

		_foundation = PxCreateFoundation(PX_FOUNDATION_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
		RN_ASSERT(_foundation, "PxCreateFoundation failed!");

		if(debug)
		{
			_pvd = physx::PxCreatePvd(*_foundation);
			physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("localhost", 5435, 100); //First parameter is ip of system running the physx visual debugger
			_pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
		}

		bool recordMemoryAllocations = true;
		physx::PxTolerancesScale scale;
		_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *_foundation, scale, recordMemoryAllocations, _pvd);
		RN_ASSERT(_physics, "PxCreatePhysics failed!");

		RN_ASSERT(PxInitExtensions(*_physics, _pvd), "PxInitExtensions failed!");

		_cooking = PxCreateCooking(PX_PHYSICS_VERSION, *_foundation, physx::PxCookingParams(scale));
		RN_ASSERT(_cooking, "PxCreateCooking failed!");

		physx::PxSceneDesc sceneDesc(_physics->getTolerancesScale());
		sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
		_dispatcher = physx::PxDefaultCpuDispatcherCreate(2);
		sceneDesc.cpuDispatcher = _dispatcher;
		sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
		_scene = _physics->createScene(sceneDesc);

		_controllerManager = PxCreateControllerManager(*_scene);
	}

	PhysXWorld::~PhysXWorld()
	{
		//TODO: delete all collision objects
		_scene->release();
		_dispatcher->release();
		_cooking->release();
		_physics->release();

		if(_pvd)
		{
			physx::PxPvdTransport* transport = _pvd->getTransport();
			_pvd->release();
			transport->release();
		}
		
		_foundation->release();

		_sharedInstance = nullptr;
	}

	void PhysXWorld::SetGravity(const Vector3 &gravity)
	{
		Lock();
		_scene->setGravity(physx::PxVec3(gravity.x, gravity.y, gravity.z));
		Unlock();
	}

	void PhysXWorld::SetStepSize(double stepsize)
	{
		_stepSize = stepsize;
	}

	void PhysXWorld::SetPaused(bool paused)
	{
		_paused = paused;
	}

	void PhysXWorld::Update(float delta)
	{
		if(_paused)
			return;

		_remainingTime += delta;
		while(_remainingTime+k::EpsilonFloat >= _stepSize)
		{
			_remainingTime -= _stepSize;
			_scene->simulate(_stepSize);
			_scene->fetchResults(true);
		}
	}



	BulletContactInfo PhysXWorld::CastRay(const Vector3 &from, const Vector3 &to)
	{
		BulletContactInfo hit;
		return hit;
	}


	void PhysXWorld::InsertCollisionObject(PhysXCollisionObject *attachment)
	{
		Lock();
		auto iterator = _collisionObjects.find(attachment);
		if(iterator == _collisionObjects.end())
		{
			attachment->InsertIntoWorld(this);
			_collisionObjects.insert(attachment->Retain());
		}
		Unlock();
	}

	void PhysXWorld::RemoveCollisionObject(PhysXCollisionObject *attachment)
	{
		Lock();
		auto iterator = _collisionObjects.find(attachment);
		if(iterator != _collisionObjects.end())
		{
			attachment->RemoveFromWorld(this);
			_collisionObjects.erase(attachment);
			attachment->Release();
		}
		Unlock();
	}

/*	void PhysXWorld::InsertConstraint(BulletConstraint *constraint)
	{
		_dynamicsWorld->addConstraint(constraint->GetBulletConstraint());
	}*/
}
