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
#include "RNJoltKinematicController.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace RN
{
	class JoltObjectLayerMapper : public JPH::ObjectLayerPairFilter, public JPH::BroadPhaseLayerInterface, public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		JPH::ObjectLayer GetObjectLayer(uint32 collisionGroup, uint32 collisionMask, uint8 broadPhaseLayer)
		{
			uint64 upperBits = (uint64)collisionMask << 32U;
			uint64 lowerBits = (uint64)collisionGroup;
			uint64 collision = upperBits | lowerBits;
			
			size_t counter = 0;
			for(uint64 value : _objectLayerMapping)
			{
				if(value == collision)
				{
					uint16 objectLayerBits = static_cast<uint16>(counter);
					uint16 broadPhaseBits = (static_cast<uint16>(broadPhaseLayer) << 14U);
					return static_cast<JPH::ObjectLayer>(broadPhaseBits | objectLayerBits);
				}
				counter += 1;
			}
			
			_objectLayerMapping.push_back(collision);
			
			uint16 objectLayerBits = static_cast<uint16>(counter);
			uint16 broadPhaseBits = (static_cast<uint16>(broadPhaseLayer) << 14U);
			return static_cast<JPH::ObjectLayer>(broadPhaseBits | objectLayerBits);
		}
		
		uint32 GetCollisionGroup(JPH::ObjectLayer objectLayer) const
		{
			uint16 objectLayerIndex = static_cast<uint16>(objectLayer & 0b0011111111111111U);
			uint64 collision = _objectLayerMapping[static_cast<size_t>(objectLayerIndex)];
			return static_cast<uint32>(collision & 0xFFFFFFFFU);
		}
		
		uint32 GetCollisionMask(JPH::ObjectLayer objectLayer) const
		{
			uint16 objectLayerIndex = static_cast<uint16>(objectLayer & 0b0011111111111111U);
			uint64 collision = _objectLayerMapping[static_cast<size_t>(objectLayerIndex)];
			return static_cast<uint32>(collision >> 32U);
		}
		
		//From JPH::ObjectLayerPairFilter
		bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const final
		{
			uint32 collisionGroup1 = GetCollisionGroup(inObject1);
			uint32 collisionMask1 = GetCollisionMask(inObject1);
			
			uint32 collisionGroup2 = GetCollisionGroup(inObject2);
			uint32 collisionMask2 = GetCollisionMask(inObject2);
			
			bool filterMask = (collisionGroup1 & collisionMask2) && (collisionGroup2 & collisionMask1);
			//bool filterID = (filterData0.word3 == 0 && filterData1.word3 == 0) || (filterData0.word2 != filterData1.word3 && filterData0.word3 != filterData1.word2);
			return (filterMask);// && filterID)
		}
		
		//From JPH::ObjectVsBroadPhaseLayerFilter
		bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const final
		{
			/*uint16 broadPhaseLayer = (inLayer1 >> 30) & 0xf; //Encoded in the last 2 bits
			
			if(broadPhaseLayer == 0) //Is static object layer
			{
				return inLayer2 != JoltBroadPhaseLayers::LAYER_STATIC; //Only collide with none-static
			}*/
			
			return true; //Everything else collides with everything
		}
		
		//From JPH::BroadPhaseLayerInterface
		uint GetNumBroadPhaseLayers() const final
		{
			return 4;
		}

		//From JPH::BroadPhaseLayerInterface
		JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const final
		{
			uint16 broadPhaseLayer = (inLayer >> 14U) & 0xf; //Encoded in the last 2 bits
			return JPH::BroadPhaseLayer(broadPhaseLayer);
		}

	#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		//From JPH::BroadPhaseLayerInterface
		virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
		{
			switch((JPH::BroadPhaseLayer::Type)inLayer)
			{
				case 0:
					return "LAYER_0";
						
				case 1:
					return "LAYER_1";
					
				case 2:
					return "LAYER_2";
					
				case 3:
					return "LAYER_3";
					
				default:
					JPH_ASSERT(false);
					return "INVALID";
			}
		}
	#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
		
	private:
		std::vector<uint64> _objectLayerMapping;
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

	class JoltCharacterContactListener : public JPH::CharacterContactListener
	{
	public:
		/// Callback to adjust the velocity of a body as seen by the character. Can be adjusted to e.g. implement a conveyor belt or an inertial dampener system of a sci-fi space ship.
		/// Note that inBody2 is locked during the callback so you can read its properties freely.
		void OnAdjustBodyVelocity(const JPH::CharacterVirtual *inCharacter, const JPH::Body &inBody2, JPH::Vec3 &ioLinearVelocity, JPH::Vec3 &ioAngularVelocity) override;

		/// Checks if a character can collide with specified body. Return true if the contact is valid.
		bool OnContactValidate(const JPH::CharacterVirtual *inCharacter, const JPH::BodyID &inBodyID2, const JPH::SubShapeID &inSubShapeID2) override;

		/// Called whenever the character collides with a body. Returns true if the contact can push the character.
		/// @param inCharacter Character that is being solved
		/// @param inBodyID2 Body ID of body that is being hit
		/// @param inSubShapeID2 Sub shape ID of shape that is being hit
		/// @param inContactPosition World space contact position
		/// @param inContactNormal World space contact normal
		/// @param ioSettings Settings returned by the contact callback to indicate how the character should behave
		void OnContactAdded(const JPH::CharacterVirtual *inCharacter, const JPH::BodyID &inBodyID2, const JPH::SubShapeID &inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings &ioSettings) override;

		/// Called whenever a contact is being used by the solver. Allows the listener to override the resulting character velocity (e.g. by preventing sliding along certain surfaces).
		/// @param inCharacter Character that is being solved
		/// @param inBodyID2 Body ID of body that is being hit
		/// @param inSubShapeID2 Sub shape ID of shape that is being hit
		/// @param inContactPosition World space contact position
		/// @param inContactNormal World space contact normal
		/// @param inContactVelocity World space velocity of contact point (e.g. for a moving platform)
		/// @param inContactMaterial Material of contact point
		/// @param inCharacterVelocity World space velocity of the character prior to hitting this contact
		/// @param ioNewCharacterVelocity Contains the calculated world space velocity of the character after hitting this contact, this velocity slides along the surface of the contact. Can be modified by the listener to provide an alternative velocity.
		void OnContactSolve(const JPH::CharacterVirtual *inCharacter, const JPH::BodyID &inBodyID2, const JPH::SubShapeID &inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial *inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3 &ioNewCharacterVelocity) override;
		
		JoltKinematicController *controller;
	};

	struct JoltInternals
	{
		JPH::TempAllocatorImpl *tempAllocator;
		JPH::JobSystemThreadPool *jobSystem;
		
		JoltObjectLayerMapper objectLayerMapper;
		
		JoltContactListener contactListener;
	};

	struct JoltCharacterInternals
	{
		JoltCharacterContactListener contactListener;
	};
}

#endif /* defined(__RAYNE_JOLTINTERNALS_H_) */
