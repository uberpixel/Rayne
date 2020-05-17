//
//  RNPhysXInternals.h
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSXINTERNALS_H_
#define __RAYNE_PHYSXINTERNALS_H_

#include "RNPhysX.h"
#include "PxPhysicsAPI.h"

namespace RN
{
	class PhysXCallback
	{
	public:
		static physx::PxFilterFlags CollisionFilterShader(
			physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
			physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
			physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize);
	};

	class PhysXQueryFilterCallback : public physx::PxQueryFilterCallback
	{
		physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags) final;
		physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit) final;
	};

	class PhysXSimulationCallback : public physx::PxSimulationEventCallback
	{
	public:
		void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) final {}
		void onWake(physx::PxActor** actors, physx::PxU32 count) final {}
		void onSleep(physx::PxActor** actors, physx::PxU32 count) final {}
		void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) final;
		void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) final {}
		void onAdvance(const physx::PxRigidBody*const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) final {}
	};

	class PhysXKinematicControllerCallback : public physx::PxUserControllerHitReport, public physx::PxControllerFilterCallback
	{
	public:
		void onShapeHit(const physx::PxControllerShapeHit &hit) final;
		void onControllerHit(const physx::PxControllersHit& hit) final;
		void onObstacleHit(const physx::PxControllerObstacleHit &hit) final;
		bool filter(const physx::PxController& a, const physx::PxController& b) final;
	};
	
/*	class PhysXRaycastCallback : public physx::PxRaycastCallback
	{
		
	}*/
}

#endif /* defined(__RAYNE_PHYSXINTERNALS_H_) */
