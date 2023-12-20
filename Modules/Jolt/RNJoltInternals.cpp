//
//  RNJoltInternals.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltInternals.h"
#include "RNJoltCollisionObject.h"
#include "RNJoltKinematicController.h"
#include "RNJoltWorld.h"

namespace RN
{
/*	Jolt::PxFilterFlags JoltCallback::CollisionFilterShader(
		Jolt::PxFilterObjectAttributes attributes0, Jolt::PxFilterData filterData0,
		Jolt::PxFilterObjectAttributes attributes1, Jolt::PxFilterData filterData1,
		Jolt::PxPairFlags& pairFlags, const void* constantBlock, Jolt::PxU32 constantBlockSize)
	{
		// let triggers through
		if(Jolt::PxFilterObjectIsTrigger(attributes0) || Jolt::PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = Jolt::PxPairFlag::eTRIGGER_DEFAULT;
			return Jolt::PxFilterFlag::eDEFAULT;
		}

		// generate contacts for all that were not filtered above
		pairFlags = Jolt::PxPairFlag::eCONTACT_DEFAULT | Jolt::PxPairFlag::eDETECT_CCD_CONTACT | Jolt::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND | Jolt::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS | Jolt::PxPairFlag::eNOTIFY_CONTACT_POINTS;

		// trigger the contact callback for pairs (A,B) where 
		// the filtermask of A contains the ID of B and vice versa.
		bool filterMask = (filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1);
		bool filterID = (filterData0.word3 == 0 && filterData1.word3 == 0) || (filterData0.word2 != filterData1.word3 && filterData0.word3 != filterData1.word2);
		if(filterMask && filterID)
			return Jolt::PxFilterFlag::eDEFAULT;

		return Jolt::PxFilterFlag::eKILL;
	}

	Jolt::PxFilterFlags JoltCallback::VehicleFilterShader(Jolt::PxFilterObjectAttributes attributes0, Jolt::PxFilterData filterData0, Jolt::PxFilterObjectAttributes attributes1, Jolt::PxFilterData filterData1, Jolt::PxPairFlags& pairFlags, const void* constantBlock, Jolt::PxU32 constantBlockSize)
	{
		PX_UNUSED(attributes0);
		PX_UNUSED(attributes1);
		PX_UNUSED(constantBlock);
		PX_UNUSED(constantBlockSize);

		if( (0 == (filterData0.word0 & filterData1.word1)) && (0 == (filterData1.word0 & filterData0.word1)) )
			return Jolt::PxFilterFlag::eSUPPRESS;

		pairFlags = Jolt::PxPairFlag::eCONTACT_DEFAULT;
		pairFlags |= Jolt::PxPairFlags(Jolt::PxU16(filterData0.word2 | filterData1.word2));

		return Jolt::PxFilterFlags();
	}

	Jolt::PxQueryHitType::Enum JoltCallback::VehicleQueryPreFilter(Jolt::PxFilterData filterData0, Jolt::PxFilterData filterData1, const void* constantBlock, Jolt::PxU32 constantBlockSize, Jolt::PxHitFlags& queryFlags)
	{
		bool filterMask = (filterData1.word0 & filterData0.word1) && (filterData0.word0 & filterData1.word1);
		if(filterMask)
		{
			return Jolt::PxQueryHitType::eBLOCK;
		}

		return Jolt::PxQueryHitType::eNONE;
	}

	Jolt::PxQueryHitType::Enum JoltQueryFilterCallback::preFilter(const Jolt::PxFilterData& filterData, const Jolt::PxShape* shape, const Jolt::PxRigidActor* actor, Jolt::PxHitFlags& queryFlags)
	{
		const Jolt::PxFilterData &shapeFilterData = shape->getQueryFilterData();
		bool filterMask = (shapeFilterData.word0 & filterData.word1) && (filterData.word0 & shapeFilterData.word1);
		bool filterID = (shapeFilterData.word3 == 0 && filterData.word3 == 0) || (shapeFilterData.word2 != filterData.word3 && shapeFilterData.word3 != filterData.word2);
		if(filterMask && filterID)
			return Jolt::PxQueryHitType::eBLOCK;

		return Jolt::PxQueryHitType::eNONE;
	}

	Jolt::PxQueryHitType::Enum JoltQueryFilterCallback::postFilter(const Jolt::PxFilterData& filterData, const Jolt::PxQueryHit& hit)
	{
		return Jolt::PxQueryHitType::eNONE;
	}

	void JoltSimulationCallback::onContact(const Jolt::PxContactPairHeader& pairHeader, const Jolt::PxContactPair* pairs, Jolt::PxU32 nbPairs)
	{
		for(Jolt::PxU32 i = 0; i < nbPairs; i++)
		{
			const Jolt::PxContactPair &contactPair = pairs[i];

			if(contactPair.events & Jolt::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND || contactPair.events & Jolt::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS)
			{
				JoltCollisionObject::ContactState contactState = JoltCollisionObject::ContactState::Begin;
				if(contactPair.events & Jolt::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS)
				{
					contactState = JoltCollisionObject::ContactState::Continue;
				}

				JoltCollisionObject *objectA = static_cast<JoltCollisionObject*>(pairHeader.actors[0]->userData);
				JoltCollisionObject *objectB = static_cast<JoltCollisionObject*>(pairHeader.actors[1]->userData);

				Jolt::PxContactPairPoint contactPoint;
				int nbPoints = contactPair.extractContacts(&contactPoint, 1);

				if(nbPoints > 0)
				{
					if(objectA->_contactCallback)
					{
						JoltContactInfo contactInfo;
						contactInfo.distance = contactPoint.separation;
						contactInfo.node = SafeRetain(objectB->GetParent());
						contactInfo.collisionObject = objectB;

						contactInfo.normal = RN::Vector3(contactPoint.normal.x, contactPoint.normal.y, contactPoint.normal.z);
						contactInfo.position = RN::Vector3(contactPoint.position.x, contactPoint.position.y, contactPoint.position.z);
						objectA->_contactCallback(objectB, contactInfo, contactState);
						SafeRelease(contactInfo.node);
					}

					if(objectB->_contactCallback)
					{
						JoltContactInfo contactInfo;
						contactInfo.distance = contactPoint.separation;
						contactInfo.node = SafeRetain(objectA->GetParent());
						contactInfo.collisionObject = objectA;
						contactInfo.normal = -RN::Vector3(contactPoint.normal.x, contactPoint.normal.y, contactPoint.normal.z);
						contactInfo.position = RN::Vector3(contactPoint.position.x, contactPoint.position.y, contactPoint.position.z);
						objectB->_contactCallback(objectA, contactInfo, contactState);
						SafeRelease(contactInfo.node);
					}
				}
			}
		}
	}*/

	void JoltCharacterContactListener::OnAdjustBodyVelocity(const JPH::CharacterVirtual *inCharacter, const JPH::Body &inBody2, JPH::Vec3 &ioLinearVelocity, JPH::Vec3 &ioAngularVelocity)
	{
		/* Do nothing, the linear and angular velocity are already filled in */
	}

	bool JoltCharacterContactListener::OnContactValidate(const JPH::CharacterVirtual *inCharacter, const JPH::BodyID &inBodyID2, const JPH::SubShapeID &inSubShapeID2)
	{
		return true;
	}

	void JoltCharacterContactListener::OnContactAdded(const JPH::CharacterVirtual *inCharacter, const JPH::BodyID &inBodyID2, const JPH::SubShapeID &inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings &ioSettings)
	{
		/* Default do nothing */
		JoltContactInfo info;
		
		info.collisionObject = reinterpret_cast<JoltCollisionObject*>(JoltWorld::GetSharedInstance()->GetJoltInstance()->GetBodyInterface().GetUserData(inBodyID2));
		if(info.collisionObject) info.node = info.collisionObject->GetParent();
		if(info.node) info.node->Retain()->Autorelease();
		
		info.position.x = inContactPosition.GetX();
		info.position.y = inContactPosition.GetY();
		info.position.z = inContactPosition.GetZ();
		info.normal.x = -inContactNormal.GetX();
		info.normal.y = -inContactNormal.GetY();
		info.normal.z = -inContactNormal.GetZ();
		info.distance = 0.0f;
		if(controller->_contactCallback) controller->_contactCallback(info.collisionObject, info);
	}

	void JoltCharacterContactListener::OnContactSolve(const JPH::CharacterVirtual *inCharacter, const JPH::BodyID &inBodyID2, const JPH::SubShapeID &inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial *inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3 &ioNewCharacterVelocity)
	{
		/* Default do nothing */
	}
}

