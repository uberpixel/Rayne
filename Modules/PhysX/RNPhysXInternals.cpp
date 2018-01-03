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

			if(contactPair.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND || contactPair.events & physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
			{
				PhysXCollisionObject::ContactState contactState = PhysXCollisionObject::ContactState::Begin;
				if(contactPair.events & physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
				{
					contactState = PhysXCollisionObject::ContactState::Continue;
				}

				PhysXCollisionObject *objectA = static_cast<PhysXCollisionObject*>(pairHeader.actors[0]->userData);
				PhysXCollisionObject *objectB = static_cast<PhysXCollisionObject*>(pairHeader.actors[1]->userData);

				if(objectA->_contactCallback)
				{
					PhysXContactInfo contactInfo;
					contactInfo.distance = 0.0f;
					contactInfo.node = objectB->GetParent();
//					contactInfo.normal = RN::Vector3(contactPair., contactPoint.m_normalWorldOnB.getY(), contactPoint.m_normalWorldOnB.getZ());
//					contactInfo.position = RN::Vector3(contactPoint.m_positionWorldOnB.getX(), contactPoint.m_positionWorldOnB.getY(), contactPoint.m_positionWorldOnB.getZ());
					objectA->_contactCallback(objectB, contactInfo, contactState);
				}

				if(objectB->_contactCallback)
				{
					PhysXContactInfo contactInfo;
					contactInfo.distance = 0.0f;
					contactInfo.node = objectA->GetParent();
//					contactInfo.normal = -RN::Vector3(contactPoint.m_normalWorldOnB.getX(), contactPoint.m_normalWorldOnB.getY(), contactPoint.m_normalWorldOnB.getZ());
//					contactInfo.position = RN::Vector3(contactPoint.m_positionWorldOnA.getX(), contactPoint.m_positionWorldOnA.getY(), contactPoint.m_positionWorldOnA.getZ());
					objectB->_contactCallback(objectA, contactInfo, contactState);
				}
			}
		}
	}
}
