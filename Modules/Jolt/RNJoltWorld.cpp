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

	JoltWorld::JoltWorld(const Vector3 &gravity, uint32 maxBodies, uint32 maxBodyPairs, uint32 maxContactConstraints) : _substeps(1), _paused(false), _isSimulating(false), _isLoadingLevel(false)
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
		_physicsSystem->Init(maxBodies, 0, maxBodyPairs, maxContactConstraints, _internals->objectLayerMapper, _internals->objectLayerMapper, _internals->objectLayerMapper);
		
		_physicsSystem->SetContactListener(&_internals->contactListener);
		
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
		SceneAttachment::Update(delta);
		
/*		if(_paused)
			return;
		
		if(delta > 0.1f || delta < k::EpsilonFloat)
			return;
		
		_isSimulating = true;
		_physicsSystem->Update(delta, _substeps, _internals->tempAllocator, _internals->jobSystem); //This waits for all jobs to finish! Maybe it can be split up into simulate and finish like with physx (wait here for all jobs, but start them in WillUpdate)
		_isSimulating = false;*/

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
		
		JPH::BodyIDVector bodyIDs;
		_physicsSystem->GetActiveBodies(JPH::EBodyType::RigidBody, bodyIDs);
		for(JPH::BodyID bodyID : bodyIDs)
		{
			uint64 userData = _physicsSystem->GetBodyInterface().GetUserData(bodyID);
			JoltCollisionObject *collisionObject = reinterpret_cast<JoltCollisionObject*>(userData);
			if(collisionObject) collisionObject->UpdatePosition();
		}
	}

	void JoltWorld::WillUpdate(float delta)
	{
		SceneAttachment::WillUpdate(delta);
		
		//_physicsSystem->OptimizeBroadPhase();
		
		if(_paused)
			return;
		
		if(delta > 0.1f || delta < k::EpsilonFloat)
			return;

		//Physics update after adding lots of objects needs to happen before doing any scene queries to prevent the quadtree from having issues...
		_isSimulating = true;
		_physicsSystem->Update(delta, _substeps, _internals->tempAllocator, _internals->jobSystem); //This waits for all jobs to finish! Maybe it can be split up into simulate and finish like with physx (wait here for all jobs, but start them in WillUpdate)
		_isSimulating = false;
	}


	void JoltWorld::PrepareLoadingLevel()
	{
		RN_DEBUG_ASSERT(!_isLoadingLevel, "Jolt is already setup for level loading! PrepareLoadingLevel should only be called once!");
		_isLoadingLevel = true;
	}

	void JoltWorld::FinalizeLoadingLevel()
	{
		RN_DEBUG_ASSERT(_isLoadingLevel, "PrepareLoadingLevel was not called or loading was already finalized!");
		_isLoadingLevel = false;
		
		if(_internals->bodiesToAddLoadingLevel.size() == 0) return;
		
		JPH::BodyInterface &bodyInterface = _physicsSystem->GetBodyInterface();
		JPH::BodyInterface::AddState state = bodyInterface.AddBodiesPrepare(_internals->bodiesToAddLoadingLevel.data(), _internals->bodiesToAddLoadingLevel.size());
		bodyInterface.AddBodiesFinalize(_internals->bodiesToAddLoadingLevel.data(), _internals->bodiesToAddLoadingLevel.size(), state, JPH::EActivation::DontActivate);
		
		_internals->bodiesToAddLoadingLevel.clear();
	}

	void JoltWorld::AddBodyForLoadingLevel(JPH::Body *body)
	{
		RN_DEBUG_ASSERT(_isLoadingLevel, "Not currently loading a level! This should only be called internally for bulk body creation!");
		_internals->bodiesToAddLoadingLevel.push_back(body->GetID());
	}


	uint16 JoltWorld::GetObjectLayer(uint32 collisionGroup, uint32 collisionMask, uint8 broadPhaseLayer)
	{
		return _internals->objectLayerMapper.GetObjectLayer(collisionGroup, collisionMask, broadPhaseLayer);
	}


	JoltContactInfo JoltWorld::CastRay(const Vector3 &from, const Vector3 &to, uint32 filterGroup, uint32 filterMask)
	{
		JoltContactInfo hit;
		hit.distance = -1.0f;
		hit.node = nullptr;
		hit.collisionObject = nullptr;

        Vector3 diff = to - from;
		
		//TODO: Limit max distance of raycast or the result
		
		JPH::RRayCast rayInfo;
		rayInfo.mOrigin = JPH::Vec3(from.x, from.y, from.z);
		rayInfo.mDirection = JPH::Vec3(diff.x, diff.y, diff.z);
		
		JPH::RayCastResult result;
		uint16 objectLayer = GetObjectLayer(filterGroup, filterMask, 1);
		if(!_physicsSystem->GetNarrowPhaseQuery().CastRay(rayInfo, result, _physicsSystem->GetDefaultBroadPhaseLayerFilter(objectLayer), _physicsSystem->GetDefaultLayerFilter(objectLayer)))
		{
			return hit;
		}
		
		JPH::Vec3 position = rayInfo.GetPointOnRay(result.mFraction);
		JPH::Vec3 normal;

		// Scoped lock
		{
			JPH::BodyLockRead lock(_physicsSystem->GetBodyLockInterface(), result.mBodyID);
			if(lock.Succeeded()) // bodyID may no longer be valid
			{
				const JPH::Body &body = lock.GetBody();
				normal = body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, position);
				hit.collisionObject = reinterpret_cast<JoltCollisionObject*>(body.GetUserData());
			}
			else
			{
				return hit;
			}
		}
		
		hit.position.x = position.GetX();
		hit.position.y = position.GetY();
		hit.position.z = position.GetZ();
		
		hit.normal.x = normal.GetX();
		hit.normal.y = normal.GetY();
		hit.normal.z = normal.GetZ();

		hit.distance = from.GetDistance(hit.position);
		
		if(hit.collisionObject) hit.node = hit.collisionObject->GetParent();
		if(hit.node) hit.node->Retain()->Autorelease();
		
		return hit;
	}

	JoltContactInfo JoltWorld::CastSweep(JoltShape *shape, const Quaternion &rotation, const Vector3 &from, const Vector3 &to, const Vector3 &scale, uint32 filterGroup, uint32 filterMask)
	{
		JoltContactInfo hit;
		hit.distance = -1.0f;
		hit.node = nullptr;
		hit.collisionObject = nullptr;

		Vector3 diff = to - from;
		
		JPH::Mat44 worldTransform = JPH::Mat44::sRotationTranslation(JPH::QuatArg(rotation.x, rotation.y, rotation.z, rotation.w), JPH::Vec3Arg(from.x, from.y, from.z));
		
		//TODO: Limit max distance of raycast or the result
		
		JPH::RShapeCast castInfo = JPH::RShapeCast::sFromWorldTransform(shape->GetJoltShape(), JPH::Vec3Arg(scale.x, scale.y, scale.z), worldTransform, JPH::Vec3Arg(diff.x, diff.y, diff.z));
		
		JPH::ShapeCastSettings castSettings; //Defaults seem ok for now!?
		
		uint16 objectLayer = GetObjectLayer(filterGroup, filterMask, 1);
		JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> result;
		_physicsSystem->GetNarrowPhaseQuery().CastShape(castInfo, castSettings, JPH::RVec3Arg(0, 0, 0), result, _physicsSystem->GetDefaultBroadPhaseLayerFilter(objectLayer), _physicsSystem->GetDefaultLayerFilter(objectLayer));
		if(!result.HadHit())
		{
			return hit;
		}
		
		JPH::Vec3 position = castInfo.GetPointOnRay(result.mHit.mFraction);
		JPH::Vec3 normal;

		// Scoped lock
		{
			JPH::BodyLockRead lock(_physicsSystem->GetBodyLockInterface(), result.mHit.mBodyID2);
			if(lock.Succeeded()) // bodyID may no longer be valid
			{
				const JPH::Body &body = lock.GetBody();
				normal = body.GetWorldSpaceSurfaceNormal(result.mHit.mSubShapeID2, position);
				hit.collisionObject = reinterpret_cast<JoltCollisionObject*>(body.GetUserData());
			}
			else
			{
				return hit;
			}
		}
		
		hit.position.x = position.GetX();
		hit.position.y = position.GetY();
		hit.position.z = position.GetZ();
		
		hit.normal.x = normal.GetX();
		hit.normal.y = normal.GetY();
		hit.normal.z = normal.GetZ();

		hit.distance = from.GetDistance(hit.position);
		
		if(hit.collisionObject) hit.node = hit.collisionObject->GetParent();
		if(hit.node) hit.node->Retain()->Autorelease();
		
		return hit;
	}

	std::vector<JoltContactInfo> JoltWorld::CheckOverlap(JoltShape *shape, const Vector3 &position, const Quaternion &rotation, const Vector3 &scale, uint32 filterGroup, uint32 filterMask)
	{
		std::vector<JoltContactInfo> hits;

		JPH::Mat44 worldTransform = JPH::Mat44::sRotationTranslation(JPH::QuatArg(rotation.x, rotation.y, rotation.z, rotation.w), JPH::Vec3Arg(position.x, position.y, position.z));
		JPH::CollideShapeSettings collideSettings; //Defaults seem ok for now!?
		
		JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> results;
		uint16 objectLayer = GetObjectLayer(filterGroup, filterMask, 1);
		_physicsSystem->GetNarrowPhaseQuery().CollideShape(shape->GetJoltShape(), JPH::Vec3Arg(scale.x, scale.y, scale.z), worldTransform.PreTranslated(shape->GetJoltShape()->GetCenterOfMass()), collideSettings, JPH::RVec3Arg(0, 0, 0), results, _physicsSystem->GetDefaultBroadPhaseLayerFilter(objectLayer), _physicsSystem->GetDefaultLayerFilter(objectLayer));
		
		for(auto result : results.mHits)
		{
			JoltContactInfo hit;
			hit.distance = 0.0f;
			hit.position = position;
			hit.node = nullptr;
			hit.collisionObject = nullptr;
			
			hit.collisionObject = reinterpret_cast<JoltCollisionObject*>(_physicsSystem->GetBodyInterface().GetUserData(result.mBodyID2));
			if(hit.collisionObject) hit.node = hit.collisionObject->GetParent();
			if(hit.node) hit.node->Retain()->Autorelease();

			hits.push_back(hit);
		}
		
		return hits;
	}
}
