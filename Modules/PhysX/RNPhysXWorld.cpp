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

	PhysXWorld::PhysXWorld(const Vector3 &gravity, String *pvdServerIP) : _pvd(nullptr), _hasVehicles(false), _substeps(1), _paused(false), _isSimulating(false)
	{
		RN_ASSERT(!_sharedInstance, "There can only be one PhysX instance at a time!");
		_sharedInstance = this;

		_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
		RN_ASSERT(_foundation, "PxCreateFoundation failed!");

		if(pvdServerIP)
		{
			_pvd = physx::PxCreatePvd(*_foundation);
			physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate(pvdServerIP->GetUTF8String(), 5425, 100); //First parameter is ip of system running the physx visual debugger
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
		sceneDesc.solverType = physx::PxSolverType::eTGS; //Enables the better, but somewhat slower solver
		sceneDesc.gravity = physx::PxVec3(gravity.x, gravity.y, gravity.z);
		_dispatcher = physx::PxDefaultCpuDispatcherCreate(2);
		sceneDesc.cpuDispatcher = _dispatcher;
		sceneDesc.filterShader = PhysXCallback::CollisionFilterShader;
		sceneDesc.simulationEventCallback = _simulationCallback;
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS | physx::PxSceneFlag::eENABLE_CCD /*| physx::PxSceneFlag::eREQUIRE_RW_LOCK*/ | physx::PxSceneFlag::eADAPTIVE_FORCE/* | physx::PxSceneFlag::eENABLE_STABILIZATION*/; // adaptive force and stabilization can not both be enabled!
		_scene = _physics->createScene(sceneDesc);

		physx::PxPvdSceneClient* pvdClient = _scene->getScenePvdClient();
		if(pvdClient)
		{
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}

		_controllerManagerFilterCallback = new PhysXKinematicControllerCallback();
		_controllerManager = PxCreateControllerManager(*_scene);
	}

	PhysXWorld::~PhysXWorld()
	{
		//TODO: delete all collision objects
		_controllerManager->release();
		delete _controllerManagerFilterCallback;
		_scene->release();
		_dispatcher->release();
		delete _simulationCallback;
		_cooking->release();
		if(_hasVehicles)
		{
			physx::PxCloseVehicleSDK();
		}
		PxCloseExtensions();
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

	void PhysXWorld::InitializeVehicles()
	{
		PxInitVehicleSDK(*_physics);
		PxVehicleSetBasisVectors(physx::PxVec3(0.0f, 1.0f, 0.0f), physx::PxVec3(0.0f, 0.0f, -1.0f));
		PxVehicleSetUpdateMode(physx::PxVehicleUpdateMode::eVELOCITY_CHANGE);
		
		_hasVehicles = true;
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

		if(_substeps > 1 && delta > RN::k::EpsilonFloat)
		{
			for(int i = 0; i < _substeps; i++)
			{
				_isSimulating = true;
				_scene->simulate(delta/static_cast<double>(_substeps));	//TODO: Fix this to use fixed steps with interpolation...
				_scene->fetchResults(true);
				_isSimulating = false;
			}
		}
		else if(_substeps == 1 && _isSimulating)
		{
			_scene->fetchResults(true); //This blocks and waits for the physics simulation to finish
			_isSimulating = false;
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

	void PhysXWorld::WillUpdate(float delta)
	{
		SceneAttachment::WillUpdate(delta);

		if(delta > RN::k::EpsilonFloat)
		{
			_controllerManager->computeInteractions(delta, _controllerManagerFilterCallback);

			if(_substeps == 1 && !_isSimulating)
			{
				_isSimulating = true;
				_scene->simulate(delta / static_cast<double>(_substeps)); //This returns immediately and kicks off the physics simulation BEFORE updating any scene nodes, this makes the physics simulation lag behind one frame, but allows running it in parallel to the scene node updates. Direct transform changes will immediately be reflected, others only a frame later
			}
		}
	}


	PhysXContactInfo PhysXWorld::CastRay(const Vector3 &from, const Vector3 &to, uint32 filterGroup, uint32 filterMask)
	{
		PhysXContactInfo hit;
		hit.distance = -1.0f;
		hit.node = nullptr;
		hit.collisionObject = nullptr;
		
		Vector3 diff = to-from;
		float distance = diff.GetLength();
		diff.Normalize();
		physx::PxRaycastBuffer callback;
		physx::PxFilterData filterData;
        filterData.word0 = filterGroup;
		filterData.word1 = filterMask;
		filterData.word2 = 0;
		filterData.word3 = 0;
		PhysXQueryFilterCallback queryFilter;
		bool didHit = _scene->raycast(physx::PxVec3(from.x, from.y, from.z), physx::PxVec3(diff.x, diff.y, diff.z), distance, callback, physx::PxHitFlags(physx::PxHitFlag::eDEFAULT), physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eDYNAMIC|physx::PxQueryFlag::eSTATIC|physx::PxQueryFlag::ePREFILTER), &queryFilter);
		
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
					hit.collisionObject = static_cast<PhysXCollisionObject*>(userData);
					hit.node = hit.collisionObject->GetParent();
					if(hit.node) hit.node->Retain()->Autorelease();
				}
			}
		}
		
		return hit;
	}

	PhysXContactInfo PhysXWorld::CastSweep(PhysXShape *shape, const Quaternion &rotation, const Vector3 &from, const Vector3 &to, float inflation, uint32 filterGroup, uint32 filterMask)
	{
		PhysXContactInfo hit;
		hit.distance = -1.0f;
		hit.node = nullptr;
		hit.collisionObject = nullptr;
		
		Vector3 diff = to-from;
		float distance = diff.GetLength();
		diff.Normalize();
		physx::PxSweepBuffer callback;
		physx::PxFilterData filterData;
		filterData.word0 = filterGroup;
		filterData.word1 = filterMask;
		filterData.word2 = 0;
		filterData.word3 = 0;
		PhysXQueryFilterCallback queryFilter;
		
		physx::PxTransform pose = physx::PxTransform(physx::PxVec3(from.x, from.y, from.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w));
		if(shape->GetPhysXShape())
		{
			if(_scene->sweep(shape->GetPhysXShape()->getGeometry().any(), pose, physx::PxVec3(diff.x, diff.y, diff.z), distance, callback, physx::PxHitFlags(physx::PxHitFlag::eDEFAULT), physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eDYNAMIC|physx::PxQueryFlag::eSTATIC|physx::PxQueryFlag::ePREFILTER), &queryFilter, 0, inflation))
			{
				hit.distance = callback.block.distance;
				hit.position.x = callback.block.position.x;
				hit.position.y = callback.block.position.y;
				hit.position.z = callback.block.position.z;
				hit.normal.x = callback.block.normal.x;
				hit.normal.y = callback.block.normal.y;
				hit.normal.z = callback.block.normal.z;
				
				if(callback.block.actor)
				{
					void *userData = callback.block.actor->userData;
					if(userData)
					{
						hit.collisionObject = static_cast<PhysXCollisionObject*>(userData);
						hit.node = hit.collisionObject->GetParent();
						if(hit.node) hit.node->Retain()->Autorelease();
					}
				}
			}
		}
		else
		{
			RNDebug("CastSweep does not currently support this shape type!");
		}
		
		return hit;
	}

	std::vector<PhysXContactInfo> PhysXWorld::CheckOverlap(PhysXShape *shape, const Vector3 &position, const Quaternion &rotation, float inflation, uint32 filterGroup, uint32 filterMask, uint32 maxNumberOfOverlaps)
	{
		const physx::PxU32 bufferSize = maxNumberOfOverlaps;
		physx::PxSweepHit *hitBuffer = new physx::PxSweepHit[bufferSize];
		physx::PxSweepBuffer callback(hitBuffer, bufferSize);
		
		physx::PxFilterData filterData;
		filterData.word0 = filterGroup;
		filterData.word1 = filterMask;
		filterData.word2 = 0;
		filterData.word3 = 0;
		PhysXQueryFilterCallback queryFilter;
		physx::PxTransform pose = physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w));
		std::vector<PhysXContactInfo> results;
		if(shape->GetPhysXShape())
		{
			if(_scene->sweep(shape->GetPhysXShape()->getGeometry().any(), pose, physx::PxVec3(0.0f, 1.0f, 0.0f), 0.0f, callback, physx::PxHitFlags(physx::PxHitFlag::eDEFAULT), physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eDYNAMIC|physx::PxQueryFlag::eSTATIC|physx::PxQueryFlag::eNO_BLOCK|physx::PxQueryFlag::ePREFILTER), &queryFilter, 0, inflation))
			{
				for(uint32 i = 0; i < callback.nbTouches; i++)
				{
					PhysXContactInfo hit;
					hit.distance = 0.0f;
					hit.node = nullptr;
					hit.collisionObject = nullptr;
					hit.position = position;

					if(callback.touches[i].actor)
					{
						void *userData = callback.touches[i].actor->userData;
						if(userData)
						{
							hit.collisionObject = static_cast<PhysXCollisionObject*>(userData);
							hit.node = hit.collisionObject->GetParent();
							if(hit.node) hit.node->Retain()->Autorelease();
						}
					}

					results.push_back(hit);
				}
			}
		}
		else if(shape->IsKindOfClass(PhysXCompoundShape::GetMetaClass()))
		{
			PhysXCompoundShape *compoundShape = shape->Downcast<PhysXCompoundShape>();
			for(size_t i = 0; i < compoundShape->GetNumberOfShapes(); i++)
			{
				PhysXShape *tempShape = compoundShape->GetShape(i);
				if(_scene->sweep(tempShape->GetPhysXShape()->getGeometry().any(), pose, physx::PxVec3(0.0f, 1.0f, 0.0f), 0.0f, callback, physx::PxHitFlags(physx::PxHitFlag::eDEFAULT), physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eDYNAMIC|physx::PxQueryFlag::eSTATIC|physx::PxQueryFlag::eNO_BLOCK|physx::PxQueryFlag::ePREFILTER), 0, 0, inflation))
				{
					for(uint32 i = 0; i < callback.nbTouches; i++)
					{
						PhysXContactInfo hit;
						hit.distance = 0.0f;
						hit.node = nullptr;
						hit.collisionObject = nullptr;
						hit.position = position;

						if(callback.touches[i].actor)
						{
							void *userData = callback.touches[i].actor->userData;
							if(userData)
							{
								hit.collisionObject = static_cast<PhysXCollisionObject*>(userData);
								hit.node = hit.collisionObject->GetParent();
								if(hit.node) hit.node->Retain()->Autorelease();
							}
						}

						results.push_back(hit);
					}
				}
			}
		}
		else
		{
			RNDebug("CheckOverlap does not currently support this shape type!");
		}

		delete[] hitBuffer;
		
		return results;
	}
}
