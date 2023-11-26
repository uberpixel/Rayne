//
//  RNJoltWorld.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltWorld.h"
#include "RNJoltInternals.h"

namespace RN
{
	RNDefineMeta(JoltWorld, SceneAttachment)

	JoltWorld *JoltWorld::_sharedInstance = nullptr;

	JoltWorld::JoltWorld(const Vector3 &gravity, uint32 maxBodies, uint32 maxBodyPairs, uint32 maxContactConstraints) : _substeps(1), _paused(false), _isSimulating(false)
	{
		RN_ASSERT(!_sharedInstance, "There can only be one Jolt instance at a time!");
		_sharedInstance = this;
		
		// Register allocation hook
		JPH::RegisterDefaultAllocator();
		
		// Create a factory
		JPH::Factory::sInstance = new JPH::Factory();

		// Register all Jolt physics types
		JPH::RegisterTypes();
		
		_internals->tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024); //Preallocate 10mb for temp allocations during physics update

		// We need a job system that will execute physics jobs on multiple threads. Typically
		// you would implement the JobSystem interface yourself and let Jolt Physics run on top
		// of your own job scheduler. JobSystemThreadPool is an example implementation.
		_internals->jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

		_physicsSystem = new JPH::PhysicsSystem();
		_physicsSystem->Init(maxBodies, 0, maxBodyPairs, maxContactConstraints, _internals->broadPhaseLayerInterface, _internals->objectVsBroadPhaseLayerFilter, _internals->objectLayerPairFilter);
		
		SetGravity(gravity);
	}

	JoltWorld::~JoltWorld()
	{
		delete _physicsSystem;
		
		delete _internals->tempAllocator;
		delete _internals->jobSystem;
		
		// Unregisters all types with the factory and cleans up the default material
		JPH::UnregisterTypes();

		// Destroy the factory
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
		
		_sharedInstance = nullptr;
	}

	void JoltWorld::SetGravity(const Vector3 &gravity)
	{
		_physicsSystem->SetGravity(JPH::Vec3Arg(gravity.x, gravity.y, gravity.z));
	}

	Vector3 JoltWorld::GetGravity()
	{
		JPH::Vec3 gravity = _physicsSystem->GetGravity();
		return Vector3(gravity.GetX(), gravity.GetY(), gravity.GetZ());
	}

	void JoltWorld::SetSubsteps(uint8 substeps)
	{
		_substeps = substeps;
	}

	void JoltWorld::SetPaused(bool paused)
	{
		_paused = paused;
	}

	void JoltWorld::Update(float delta)
	{
		if(_paused)
			return;
		
		if(delta > 0.1f || delta < k::EpsilonFloat)
			return;
		
		_isSimulating = true;
		_physicsSystem->Update(delta, _substeps, _internals->tempAllocator, _internals->jobSystem); //This waits for all jobs to finish! Maybe it can be split up into simulate and finish like with physx (wait here for all jobs, but start them in WillUpdate)
		_isSimulating = false;

/*
		Jolt::PxU32 actorCount = 0;
		Jolt::PxActor **actors = _scene->getActiveActors(actorCount);
		for(int i = 0; i < actorCount; i++)
		{
			void *userData = actors[i]->userData;
			if(userData)
			{
				JoltCollisionObject *collisionObject = static_cast<JoltCollisionObject*>(userData);
				collisionObject->UpdatePosition();
			}
		}*/
	}

	void JoltWorld::WillUpdate(float delta)
	{
		SceneAttachment::WillUpdate(delta);
	}


	JoltContactInfo JoltWorld::CastRay(const Vector3 &from, const Vector3 &to, uint32 filterGroup, uint32 filterMask)
	{
		JoltContactInfo hit;
		hit.distance = -1.0f;
		hit.node = nullptr;
		hit.collisionObject = nullptr;
		
		/*Vector3 diff = to-from;
		float distance = diff.GetLength();
		diff.Normalize();
		Jolt::PxRaycastBuffer callback;
		Jolt::PxFilterData filterData;
        filterData.word0 = filterGroup;
		filterData.word1 = filterMask;
		filterData.word2 = 0;
		filterData.word3 = 0;
		JoltQueryFilterCallback queryFilter;
		bool didHit = _scene->raycast(Jolt::PxVec3(from.x, from.y, from.z), Jolt::PxVec3(diff.x, diff.y, diff.z), distance, callback, Jolt::PxHitFlags(Jolt::PxHitFlag::eDEFAULT), Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eDYNAMIC|Jolt::PxQueryFlag::eSTATIC|Jolt::PxQueryFlag::ePREFILTER), &queryFilter);
		
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
					hit.collisionObject = static_cast<JoltCollisionObject*>(userData);
					hit.node = hit.collisionObject->GetParent();
					if(hit.node) hit.node->Retain()->Autorelease();
				}
			}
		}*/
		
		return hit;
	}

	JoltContactInfo JoltWorld::CastSweep(JoltShape *shape, const Quaternion &rotation, const Vector3 &from, const Vector3 &to, float inflation, uint32 filterGroup, uint32 filterMask)
	{
		JoltContactInfo hit;
		hit.distance = -1.0f;
		hit.node = nullptr;
		hit.collisionObject = nullptr;
		
		/*Vector3 diff = to-from;
		float distance = diff.GetLength();
		diff.Normalize();
		Jolt::PxSweepBuffer callback;
		Jolt::PxFilterData filterData;
		filterData.word0 = filterGroup;
		filterData.word1 = filterMask;
		filterData.word2 = 0;
		filterData.word3 = 0;
		JoltQueryFilterCallback queryFilter;
		
		Jolt::PxTransform pose = Jolt::PxTransform(Jolt::PxVec3(from.x, from.y, from.z), Jolt::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w));
		if(shape->GetJoltShape())
		{
			if(_scene->sweep(shape->GetJoltShape()->getGeometry().any(), pose, Jolt::PxVec3(diff.x, diff.y, diff.z), distance, callback, Jolt::PxHitFlags(Jolt::PxHitFlag::eDEFAULT), Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eDYNAMIC|Jolt::PxQueryFlag::eSTATIC|Jolt::PxQueryFlag::ePREFILTER), &queryFilter, 0, inflation))
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
						hit.collisionObject = static_cast<JoltCollisionObject*>(userData);
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
		*/
		return hit;
	}

	std::vector<JoltContactInfo> JoltWorld::CheckOverlap(JoltShape *shape, const Vector3 &position, const Quaternion &rotation, float inflation, uint32 filterGroup, uint32 filterMask)
	{
	/*	const Jolt::PxU32 bufferSize = 256;
		Jolt::PxSweepHit hitBuffer[bufferSize];
		Jolt::PxSweepBuffer callback(hitBuffer, bufferSize);
		
		Jolt::PxFilterData filterData;
		filterData.word0 = filterGroup;
		filterData.word1 = filterMask;
		filterData.word2 = 0;
		filterData.word3 = 0;
		JoltQueryFilterCallback queryFilter;
		Jolt::PxTransform pose = Jolt::PxTransform(Jolt::PxVec3(position.x, position.y, position.z), Jolt::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w));
	 */
		std::vector<JoltContactInfo> results;
	/*	if(shape->GetJoltShape())
		{
			if(_scene->sweep(shape->GetJoltShape()->getGeometry().any(), pose, Jolt::PxVec3(0.0f, 1.0f, 0.0f), 0.0f, callback, Jolt::PxHitFlags(Jolt::PxHitFlag::eDEFAULT), Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eDYNAMIC|Jolt::PxQueryFlag::eSTATIC|Jolt::PxQueryFlag::eNO_BLOCK|Jolt::PxQueryFlag::ePREFILTER), &queryFilter, 0, inflation))
			{
				for(uint32 i = 0; i < callback.nbTouches; i++)
				{
					JoltContactInfo hit;
					hit.distance = 0.0f;
					hit.node = nullptr;
					hit.collisionObject = nullptr;
					hit.position = position;

					if(callback.touches[i].actor)
					{
						void *userData = callback.touches[i].actor->userData;
						if(userData)
						{
							hit.collisionObject = static_cast<JoltCollisionObject*>(userData);
							hit.node = hit.collisionObject->GetParent();
							if(hit.node) hit.node->Retain()->Autorelease();
						}
					}

					results.push_back(hit);
				}
			}
		}
		else if(shape->IsKindOfClass(JoltCompoundShape::GetMetaClass()))
		{
			JoltCompoundShape *compoundShape = shape->Downcast<JoltCompoundShape>();
			for(size_t i = 0; i < compoundShape->GetNumberOfShapes(); i++)
			{
				JoltShape *tempShape = compoundShape->GetShape(i);
				if(_scene->sweep(tempShape->GetJoltShape()->getGeometry().any(), pose, Jolt::PxVec3(0.0f, 1.0f, 0.0f), 0.0f, callback, Jolt::PxHitFlags(Jolt::PxHitFlag::eDEFAULT), Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eDYNAMIC|Jolt::PxQueryFlag::eSTATIC|Jolt::PxQueryFlag::eNO_BLOCK|Jolt::PxQueryFlag::ePREFILTER), 0, 0, inflation))
				{
					for(uint32 i = 0; i < callback.nbTouches; i++)
					{
						JoltContactInfo hit;
						hit.distance = 0.0f;
						hit.node = nullptr;
						hit.collisionObject = nullptr;
						hit.position = position;

						if(callback.touches[i].actor)
						{
							void *userData = callback.touches[i].actor->userData;
							if(userData)
							{
								hit.collisionObject = static_cast<JoltCollisionObject*>(userData);
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
		}*/
		
		return results;
	}
}
