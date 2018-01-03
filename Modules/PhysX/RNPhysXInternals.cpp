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
					if (objectA->_contactCallback)
					{
						PhysXContactInfo contactInfo;
						contactInfo.distance = contactPoint.separation;
						contactInfo.node = objectB->GetParent();

						contactInfo.normal = RN::Vector3(contactPoint.normal.x, contactPoint.normal.y, contactPoint.normal.z);
						contactInfo.position = RN::Vector3(contactPoint.position.x, contactPoint.position.y, contactPoint.position.z);
						objectA->_contactCallback(objectB, contactInfo, contactState);
					}

					if (objectB->_contactCallback)
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
