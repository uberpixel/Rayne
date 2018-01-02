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
}

#endif /* defined(__RAYNE_PHYSXINTERNALS_H_) */
