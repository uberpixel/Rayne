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
/*
namespace RN
{
	Jolt::PxFilterFlags JoltCallback::CollisionFilterShader(
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
	}

	void JoltKinematicControllerCallback::onShapeHit(const Jolt::PxControllerShapeHit &hit)
	{
		JoltCollisionObject *objectA = static_cast<JoltCollisionObject*>(hit.controller->getUserData());
		JoltCollisionObject *objectB = static_cast<JoltCollisionObject*>(hit.actor->userData);
		if(objectA->_contactCallback)
		{
			JoltContactInfo contactInfo;
			contactInfo.distance = 0.0f;
			contactInfo.node = SafeRetain(objectB->GetParent());
			contactInfo.collisionObject = objectB;
			contactInfo.normal = RN::Vector3(hit.worldNormal.x, hit.worldNormal.y, hit.worldNormal.z);
			contactInfo.position = RN::Vector3(hit.worldPos.x, hit.worldPos.y, hit.worldPos.z);
			objectA->_contactCallback(objectB, contactInfo, JoltCollisionObject::ContactState::Begin);
			SafeRelease(contactInfo.node);
		}
	}

	void JoltKinematicControllerCallback::onControllerHit(const Jolt::PxControllersHit& hit)
	{

	}

	void JoltKinematicControllerCallback::onObstacleHit(const Jolt::PxControllerObstacleHit &hit)
	{

	}

	bool JoltKinematicControllerCallback::filter(const Jolt::PxController& a, const Jolt::PxController& b)
	{
		JoltKinematicController *controllerA = static_cast<JoltKinematicController*>(a.getUserData());
		JoltKinematicController *controllerB = static_cast<JoltKinematicController*>(b.getUserData());
		if(controllerA->GetCollisionFilterGroup() & controllerB->GetCollisionFilterMask() && controllerB->GetCollisionFilterGroup() & controllerA->GetCollisionFilterMask())
		{
			return true;
		}
		
		return false;
	}

	Jolt::PxControllerBehaviorFlags JoltKinematicControllerCallback::getBehaviorFlags(const Jolt::PxShape& shape, const Jolt::PxActor& actor)
	{
		return Jolt::PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT | Jolt::PxControllerBehaviorFlag::eCCT_SLIDE;
	}

	Jolt::PxControllerBehaviorFlags JoltKinematicControllerCallback::getBehaviorFlags(const Jolt::PxController &controller)
	{
		return Jolt::PxControllerBehaviorFlags(0);
	}

	Jolt::PxControllerBehaviorFlags JoltKinematicControllerCallback::getBehaviorFlags(const Jolt::PxObstacle &obstacle)
	{
		return Jolt::PxControllerBehaviorFlags(0);
	}
}
*/
