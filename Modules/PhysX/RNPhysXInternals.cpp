//
//  RNPhysXInternals.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXInternals.h"
#include "RNPhysXCollisionObject.h"

namespace RN
{
	physx::PxFilterFlags PhysXCallback::CollisionFilterShader(
		physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
		physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
	{
		// let triggers through
		if(physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
			return physx::PxFilterFlag::eDEFAULT;
		}

		// generate contacts for all that were not filtered above
		pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT | physx::PxPairFlag::eDETECT_CCD_CONTACT | physx::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND | physx::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS | physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;

		// trigger the contact callback for pairs (A,B) where 
		// the filtermask of A contains the ID of B and vice versa.
		bool filterMask = (filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1);
		bool filterID = (filterData0.word3 == 0 && filterData1.word3 == 0) || (filterData0.word2 != filterData1.word3 && filterData0.word3 != filterData1.word2);
		if(filterMask && filterID)
			return physx::PxFilterFlag::eDEFAULT;

		return physx::PxFilterFlag::eKILL;
	}

	physx::PxQueryHitType::Enum PhysXQueryFilterCallback::preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags)
	{
		const physx::PxFilterData &shapeFilterData = shape->getQueryFilterData();
		bool filterMask = (shapeFilterData.word0 & filterData.word1) && (filterData.word0 & shapeFilterData.word1);
		bool filterID = (shapeFilterData.word3 == 0 && filterData.word3 == 0) || (shapeFilterData.word2 != filterData.word3 && shapeFilterData.word3 != filterData.word2);
		if(filterMask && filterID)
			return physx::PxQueryHitType::eBLOCK;

		return physx::PxQueryHitType::eNONE;
	}

	physx::PxQueryHitType::Enum PhysXQueryFilterCallback::postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit)
	{
		return physx::PxQueryHitType::eNONE;
	}

	void PhysXSimulationCallback::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
	{
		for(physx::PxU32 i = 0; i < nbPairs; i++)
		{
			const physx::PxContactPair &contactPair = pairs[i];

			if(contactPair.events & physx::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_FOUND || contactPair.events & physx::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS)
			{
				PhysXCollisionObject::ContactState contactState = PhysXCollisionObject::ContactState::Begin;
				if(contactPair.events & physx::PxPairFlag::eNOTIFY_THRESHOLD_FORCE_PERSISTS)
				{
					contactState = PhysXCollisionObject::ContactState::Continue;
				}

				PhysXCollisionObject *objectA = static_cast<PhysXCollisionObject*>(pairHeader.actors[0]->userData);
				PhysXCollisionObject *objectB = static_cast<PhysXCollisionObject*>(pairHeader.actors[1]->userData);

				physx::PxContactPairPoint contactPoint;
				int nbPoints = contactPair.extractContacts(&contactPoint, 1);

				if(nbPoints > 0)
				{
					if(objectA->_contactCallback)
					{
						PhysXContactInfo contactInfo;
						contactInfo.distance = contactPoint.separation;
						contactInfo.node = objectB->GetParent();

						contactInfo.normal = RN::Vector3(contactPoint.normal.x, contactPoint.normal.y, contactPoint.normal.z);
						contactInfo.position = RN::Vector3(contactPoint.position.x, contactPoint.position.y, contactPoint.position.z);
						objectA->_contactCallback(objectB, contactInfo, contactState);
					}

					if(objectB->_contactCallback)
					{
						PhysXContactInfo contactInfo;
						contactInfo.distance = contactPoint.separation;
						contactInfo.node = objectA->GetParent();
						contactInfo.normal = -RN::Vector3(contactPoint.normal.x, contactPoint.normal.y, contactPoint.normal.z);
						contactInfo.position = RN::Vector3(contactPoint.position.x, contactPoint.position.y, contactPoint.position.z);
						objectB->_contactCallback(objectA, contactInfo, contactState);
					}
				}
			}
		}
	}
}
