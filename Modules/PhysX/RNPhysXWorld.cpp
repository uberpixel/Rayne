//
//  RNPhysXWorld.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXWorld.h"
#include "PxPhysicsAPI.h"
#include "RNPhysXInternals.h"

namespace RN
{
	RNDefineMeta(PhysXWorld, SceneAttachment)

	physx::PxDefaultErrorCallback gDefaultErrorCallback;
	physx::PxDefaultAllocator gDefaultAllocatorCallback;

	PhysXWorld *PhysXWorld::_sharedInstance = nullptr;

	PhysXWorld::PhysXWorld(const Vector3 &gravity, bool debug) : _pvd(nullptr), _substeps(1), _paused(false)
	{
		RN_ASSERT(!_sharedInstance, "There can only be one PhysX instance at a time!");
		_sharedInstance = this;

		_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
		RN_ASSERT(_foundation, "PxCreateFoundation failed!");

		if(debug)
		{
			_pvd = physx::PxCreatePvd(*_foundation);
			physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 100); //First parameter is ip of system running the physx visual debugger
			_pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
		}

		bool recordMemoryAllocations = true;
		physx::PxTolerancesScale scale;
		_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *_foundation, scale, recordMemoryAllocations, _pvd);
		RN_ASSERT(_physics, "PxCreatePhysics failed!");

		RN_ASSERT(PxInitExtensions(*_physics, _pvd), "PxInitExtensions failed!");

		_cooking = PxCreateCooking(PX_PHYSICS_VERSION, *_foundation, physx::PxCookingParams(scale));
		RN_ASSERT(_cooking, "PxCreateCooking failed!");

		_simulationCallback = new PhysXSimulationCallback();

		physx::PxSceneDesc sceneDesc(_physics->getTolerancesScale());
		sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
		_dispatcher = physx::PxDefaultCpuDispatcherCreate(2);
		sceneDesc.cpuDispatcher = _dispatcher;
		sceneDesc.filterShader = PhysXCallback::CollisionFilterShader;
		sceneDesc.simulationEventCallback = _simulationCallback;
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS | physx::PxSceneFlag::eENABLE_CCD;
		_scene = _physics->createScene(sceneDesc);

		physx::PxPvdSceneClient* pvdClient = _scene->getScenePvdClient();
		if(pvdClient)
		{
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}

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

	Vector3 PhysXWorld::GetGravity()
	{
		Lock();
		const physx::PxVec3 &gravity = _scene->getGravity();
		Unlock();

		return Vector3(gravity.x, gravity.y, gravity.z);
	}

	void PhysXWorld::SetSubsteps(uint8 substeps)
	{
		_substeps = substeps;
	}

	void PhysXWorld::SetPaused(bool paused)
	{
		_paused = paused;
	}

	void PhysXWorld::Update(float delta)
	{
		if(_paused)
			return;
		
		if(delta > 0.1f || delta < k::EpsilonFloat)
			return;

		for(int i = 0; i < _substeps; i++)
		{
			_scene->simulate(delta/static_cast<double>(_substeps));	//TODO: Fix this to use fixed steps with interpolation...
			_scene->fetchResults(true);
		}


		physx::PxU32 actorCount = 0;
		physx::PxActor **actors = _scene->getActiveActors(actorCount);
		for(int i = 0; i < actorCount; i++)
		{
			void *userData = actors[i]->userData;
			if(userData)
			{
				PhysXCollisionObject *collisionObject = static_cast<PhysXCollisionObject*>(userData);
				collisionObject->UpdatePosition();
			}
		}
	}



	PhysXContactInfo PhysXWorld::CastRay(const Vector3 &from, const Vector3 &to, uint32 filterMask)
	{
		PhysXContactInfo hit;
		hit.distance = -1.0f;
		hit.node = nullptr;
		
		Vector3 diff = to-from;
		float distance = diff.GetLength();
		diff.Normalize();
		physx::PxRaycastBuffer callback;
		physx::PxFilterData filterData;
		filterData.word0 = filterMask;
		bool didHit = _scene->raycast(physx::PxVec3(from.x, from.y, from.z), physx::PxVec3(diff.x, diff.y, diff.z), distance, callback, physx::PxHitFlags(physx::PxHitFlag::eDEFAULT), physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eDYNAMIC|physx::PxQueryFlag::eSTATIC));
		
		if(didHit)
		{
			hit.distance = callback.block.distance;
			hit.position = from + diff * hit.distance;
			hit.normal.x = callback.block.normal.x;
			hit.normal.y = callback.block.normal.y;
			hit.normal.z = callback.block.normal.z;
			
			if(callback.block.actor)
			{
				void *userData = callback.block.actor->userData;
				if(userData)
				{
					hit.node = static_cast<PhysXCollisionObject*>(userData)->GetParent();
				}
			}
		}
		
		return hit;
	}
}
