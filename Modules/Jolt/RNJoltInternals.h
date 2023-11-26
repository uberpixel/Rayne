//
//  RNJoltInternals.h
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLTINTERNALS_H_
#define __RAYNE_JOLTINTERNALS_H_

#include "RNJolt.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

namespace RN
{
	class JoltCompoundShape;

/*	class JoltCallback
	{
	public:
		static Jolt::PxFilterFlags CollisionFilterShader(
			Jolt::PxFilterObjectAttributes attributes0, Jolt::PxFilterData filterData0,
			Jolt::PxFilterObjectAttributes attributes1, Jolt::PxFilterData filterData1,
			Jolt::PxPairFlags& pairFlags, const void* constantBlock, Jolt::PxU32 constantBlockSize);
		
		static Jolt::PxFilterFlags VehicleFilterShader(Jolt::PxFilterObjectAttributes attributes0, Jolt::PxFilterData filterData0, Jolt::PxFilterObjectAttributes attributes1, Jolt::PxFilterData filterData1, Jolt::PxPairFlags& pairFlags, const void* constantBlock, Jolt::PxU32 constantBlockSize);
		static Jolt::PxQueryHitType::Enum VehicleQueryPreFilter(Jolt::PxFilterData filterData0, Jolt::PxFilterData filterData1, const void* constantBlock, Jolt::PxU32 constantBlockSize, Jolt::PxHitFlags& queryFlags);
	};

	class JoltQueryFilterCallback : public Jolt::PxQueryFilterCallback
	{
		Jolt::PxQueryHitType::Enum preFilter(const Jolt::PxFilterData& filterData, const Jolt::PxShape* shape, const Jolt::PxRigidActor* actor, Jolt::PxHitFlags& queryFlags) final;
		Jolt::PxQueryHitType::Enum postFilter(const Jolt::PxFilterData& filterData, const Jolt::PxQueryHit& hit) final;
	};

	class JoltSimulationCallback : public Jolt::PxSimulationEventCallback
	{
	public:
		void onConstraintBreak(Jolt::PxConstraintInfo* constraints, Jolt::PxU32 count) final {}
		void onWake(Jolt::PxActor** actors, Jolt::PxU32 count) final {}
		void onSleep(Jolt::PxActor** actors, Jolt::PxU32 count) final {}
		void onContact(const Jolt::PxContactPairHeader& pairHeader, const Jolt::PxContactPair* pairs, Jolt::PxU32 nbPairs) final;
		void onTrigger(Jolt::PxTriggerPair* pairs, Jolt::PxU32 count) final {}
		void onAdvance(const Jolt::PxRigidBody*const* bodyBuffer, const Jolt::PxTransform* poseBuffer, const Jolt::PxU32 count) final {}
	};

	class JoltKinematicControllerCallback : public Jolt::PxUserControllerHitReport, public Jolt::PxControllerFilterCallback, public Jolt::PxControllerBehaviorCallback, public JoltQueryFilterCallback
	{
	public:
		void onShapeHit(const Jolt::PxControllerShapeHit &hit) final;
		void onControllerHit(const Jolt::PxControllersHit& hit) final;
		void onObstacleHit(const Jolt::PxControllerObstacleHit &hit) final;
		bool filter(const Jolt::PxController& a, const Jolt::PxController& b) final;
		
		Jolt::PxControllerBehaviorFlags getBehaviorFlags(const Jolt::PxShape& shape, const Jolt::PxActor& actor) final;
		Jolt::PxControllerBehaviorFlags getBehaviorFlags(const Jolt::PxController& controller) final;
		Jolt::PxControllerBehaviorFlags getBehaviorFlags(const Jolt::PxObstacle& obstacle) final;
	};*/

	// Layer that objects can be in, determines which other objects it can collide with
	// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
	// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
	// but only if you do collision testing).
	namespace JoltObjectLayers
	{
		static constexpr JPH::ObjectLayer NON_MOVING = 0;
		static constexpr JPH::ObjectLayer MOVING = 1;
		static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
	};

	/// Class that determines if two object layers can collide
	class JoltObjectLayerPairFilter : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
		{
			switch(inObject1)
			{
				case JoltObjectLayers::NON_MOVING:
					return inObject2 == JoltObjectLayers::MOVING; // Non moving only collides with moving
					
				case JoltObjectLayers::MOVING:
					return true; // Moving collides with everything
					
				default:
					JPH_ASSERT(false);
					return false;
			}
		}
	};

	// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
	// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
	// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
	// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
	// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
	namespace JoltBroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
		static constexpr JPH::BroadPhaseLayer MOVING(1);
		static constexpr uint NUM_LAYERS(2);
	};

	// BroadPhaseLayerInterface implementation
	// This defines a mapping between object and broadphase layers.
	class JoltBroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
	{
	public:
		JoltBroadPhaseLayerInterface()
		{
			// Create a mapping table from object to broad phase layer
			mObjectToBroadPhase[JoltObjectLayers::NON_MOVING] = JoltBroadPhaseLayers::NON_MOVING;
			mObjectToBroadPhase[JoltObjectLayers::MOVING] = JoltBroadPhaseLayers::MOVING;
		}

		virtual uint GetNumBroadPhaseLayers() const override
		{
			return JoltBroadPhaseLayers::NUM_LAYERS;
		}

		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
		{
			JPH_ASSERT(inLayer < JoltObjectLayers::NUM_LAYERS);
			return mObjectToBroadPhase[inLayer];
		}

	#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
		{
			switch((JPH::BroadPhaseLayer::Type)inLayer)
			{
				case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayers::NON_MOVING:
					return "NON_MOVING";
						
				case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayers::MOVING:
					return "MOVING";
					
				default:
					JPH_ASSERT(false);
					return "INVALID";
			}
		}
	#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

	private:
		JPH::BroadPhaseLayer mObjectToBroadPhase[JoltObjectLayers::NUM_LAYERS];
	};

	/// Class that determines if an object layer can collide with a broadphase layer
	class JoltObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
		{
			switch (inLayer1)
			{
				case JoltObjectLayers::NON_MOVING:
					return inLayer2 == JoltBroadPhaseLayers::MOVING;
						
				case JoltObjectLayers::MOVING:
					return true;
						
				default:
					JPH_ASSERT(false);
					return false;
			}
		}
	};

	// An example contact listener
	class JoltContactListener : public JPH::ContactListener
	{
	public:
		// See: ContactListener
		virtual JPH::ValidateResult OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override
		{
			RNDebug("Contact validate callback");

			// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
			return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
		}

		virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
		{
			RNDebug("A contact was added");
		}

		virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
		{
			RNDebug("A contact was persisted");
		}

		virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override
		{
			RNDebug("A contact was removed");
		}
	};

	// An example activation listener
	class JoltBodyActivationListener : public JPH::BodyActivationListener
	{
	public:
		virtual void OnBodyActivated(const JPH::BodyID &inBodyID, uint64 inBodyUserData) override
		{
			RNDebug("A body got activated");
		}

		virtual void OnBodyDeactivated(const JPH::BodyID &inBodyID, uint64 inBodyUserData) override
		{
			RNDebug("A body went to sleep");
		}
	};

	struct JoltInternals
	{
		JoltObjectLayerPairFilter objectLayerPairFilter;
		JoltBroadPhaseLayerInterface broadPhaseLayerInterface;
		JoltObjectVsBroadPhaseLayerFilter objectVsBroadPhaseLayerFilter;

		JPH::TempAllocatorImpl *tempAllocator;
		JPH::JobSystemThreadPool *jobSystem;
	};
}

#endif /* defined(__RAYNE_JOLTINTERNALS_H_) */
