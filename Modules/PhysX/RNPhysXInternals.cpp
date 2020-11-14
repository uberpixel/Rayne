//
//  RNPhysXInternals.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXInternals.h"
#include "RNPhysXCollisionObject.h"
#include "RNPhysXKinematicController.h"

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

	physx::PxFilterFlags PhysXCallback::VehicleFilterShader(physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0, physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
	{
		PX_UNUSED(attributes0);
		PX_UNUSED(attributes1);
		PX_UNUSED(constantBlock);
		PX_UNUSED(constantBlockSize);

		if( (0 == (filterData0.word0 & filterData1.word1)) && (0 == (filterData1.word0 & filterData0.word1)) )
			return physx::PxFilterFlag::eSUPPRESS;

		pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;
		pairFlags |= physx::PxPairFlags(physx::PxU16(filterData0.word2 | filterData1.word2));

		return physx::PxFilterFlags();
	}

	physx::PxQueryHitType::Enum PhysXCallback::VehicleQueryPreFilter(physx::PxFilterData filterData0, physx::PxFilterData filterData1, const void* constantBlock, physx::PxU32 constantBlockSize, physx::PxHitFlags& queryFlags)
	{
		bool filterMask = (filterData1.word0 & filterData0.word1) && (filterData0.word0 & filterData1.word1);
		if(filterMask)
		{
			return physx::PxQueryHitType::eBLOCK;
		}

		return physx::PxQueryHitType::eNONE;
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

	void PhysXKinematicControllerCallback::onShapeHit(const physx::PxControllerShapeHit &hit)
	{
		PhysXCollisionObject *objectA = static_cast<PhysXCollisionObject*>(hit.controller->getUserData());
		PhysXCollisionObject *objectB = static_cast<PhysXCollisionObject*>(hit.actor->userData);
		if(objectA->_contactCallback)
		{
			PhysXContactInfo contactInfo;
			contactInfo.distance = 0.0f;
			contactInfo.node = objectB->GetParent();
			contactInfo.normal = RN::Vector3(hit.worldNormal.x, hit.worldNormal.y, hit.worldNormal.z);
			contactInfo.position = RN::Vector3(hit.worldPos.x, hit.worldPos.y, hit.worldPos.z);
			objectA->_contactCallback(objectB, contactInfo, PhysXCollisionObject::ContactState::Begin);
		}
	}

	void PhysXKinematicControllerCallback::onControllerHit(const physx::PxControllersHit& hit)
	{

	}

	void PhysXKinematicControllerCallback::onObstacleHit(const physx::PxControllerObstacleHit &hit)
	{

	}

	bool PhysXKinematicControllerCallback::filter(const physx::PxController& a, const physx::PxController& b)
	{
		PhysXKinematicController *controllerA = static_cast<PhysXKinematicController*>(a.getUserData());
		PhysXKinematicController *controllerB = static_cast<PhysXKinematicController*>(b.getUserData());
		if(controllerA->GetCollisionFilterGroup() & controllerB->GetCollisionFilterMask() && controllerB->GetCollisionFilterGroup() & controllerA->GetCollisionFilterMask())
		{
			return true;
		}
		
		return false;
	}

	physx::PxControllerBehaviorFlags PhysXKinematicControllerCallback::getBehaviorFlags(const physx::PxShape& shape, const physx::PxActor& actor)
	{
		return physx::PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT | physx::PxControllerBehaviorFlag::eCCT_SLIDE;
	}

	physx::PxControllerBehaviorFlags PhysXKinematicControllerCallback::getBehaviorFlags(const physx::PxController &controller)
	{
		return physx::PxControllerBehaviorFlags(0);
	}

	physx::PxControllerBehaviorFlags PhysXKinematicControllerCallback::getBehaviorFlags(const physx::PxObstacle &obstacle)
	{
		return physx::PxControllerBehaviorFlags(0);
	}

	void PhysXVehicleInternal::SetupWheelsSimulationData(const float wheelMass, PhysXCompoundShape *compoundShape, const float numWheels, const physx::PxVec3* wheelCenterActorOffsets, const physx::PxVec3& chassisCMOffset, const float chassisMass, physx::PxVehicleWheelsSimData* wheelsSimData, uint32 wheelRaycastGroup, uint32 wheelRaycastMask)
	{
		//Set up the wheels.
		physx::PxVehicleWheelData wheels[PX_MAX_NB_WHEELS];
		{
			//Set up the wheel data structures with mass, moi, radius, width.
			for(uint32 i = 0; i < numWheels; i++)
			{
				physx::PxShape *shape = compoundShape->GetShape(i)->GetPhysXShape();
				const physx::PxConvexMeshGeometry &meshGeometry = shape->getGeometry().convexMesh();
				const physx::PxU32 numWheelVerts = meshGeometry.convexMesh->getNbVertices();
				const physx::PxVec3* wheelVerts = meshGeometry.convexMesh->getVertices();
				physx::PxVec3 wheelMin(PX_MAX_F32, PX_MAX_F32, PX_MAX_F32);
				physx::PxVec3 wheelMax(-PX_MAX_F32, -PX_MAX_F32, -PX_MAX_F32);
				for(uint32 j = 0; j < numWheelVerts; j++)
				{
					wheelMin.x = physx::PxMin(wheelMin.x,wheelVerts[j].x);
					wheelMin.y = physx::PxMin(wheelMin.y,wheelVerts[j].y);
					wheelMin.z = physx::PxMin(wheelMin.z,wheelVerts[j].z);
					wheelMax.x = physx::PxMax(wheelMax.x,wheelVerts[j].x);
					wheelMax.y = physx::PxMax(wheelMax.y,wheelVerts[j].y);
					wheelMax.z = physx::PxMax(wheelMax.z,wheelVerts[j].z);
				}
				wheels[i].mWidth = wheelMax.x - wheelMin.x;
				wheels[i].mRadius = physx::PxMax(wheelMax.y, wheelMax.z) * 0.975f;
				
				wheels[i].mMass = wheelMass;
				wheels[i].mMOI = 0.5f * wheelMass * wheels[i].mRadius * wheels[i].mRadius;
			}

			//Enable the handbrake for the rear wheels only.
			wheels[physx::PxVehicleDrive4WWheelOrder::eREAR_LEFT].mMaxHandBrakeTorque = 4000.0f;
			wheels[physx::PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mMaxHandBrakeTorque = 4000.0f;
			
			//Enable steering for the front wheels only.
			wheels[physx::PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mMaxSteer = k::Pi * 0.3333f;
			wheels[physx::PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mMaxSteer = k::Pi * 0.3333f;
		}

		//Set up the tires.
		physx::PxVehicleTireData tires[PX_MAX_NB_WHEELS];
		{
			//Set up the tires.
			for(uint32 i = 0; i < numWheels; i++)
			{
				tires[i].mType = 0;
			}
		}

		//Set up the suspensions
		physx::PxVehicleSuspensionData suspensions[PX_MAX_NB_WHEELS];
		{
			//Compute the mass supported by each suspension spring.
			float suspSprungMasses[PX_MAX_NB_WHEELS];
			PxVehicleComputeSprungMasses
				(numWheels, wheelCenterActorOffsets,
				 chassisCMOffset, chassisMass, 1, suspSprungMasses);

			//Set the suspension data.
			for(uint32 i = 0; i < numWheels; i++)
			{
				suspensions[i].mMaxCompression = 0.3f;
				suspensions[i].mMaxDroop = 0.1f;
				suspensions[i].mSpringStrength = 35000.0f;
				suspensions[i].mSpringDamperRate = 4500.0f;
				suspensions[i].mSprungMass = suspSprungMasses[i];
			}

			//Set the camber angles.
			const float camberAngleAtRest = 0.0;
			const float camberAngleAtMaxDroop = 0.01f;
			const float camberAngleAtMaxCompression = -0.01f;
			for(uint32 i = 0; i < numWheels; i += 2)
			{
				suspensions[i + 0].mCamberAtRest =  camberAngleAtRest;
				suspensions[i + 1].mCamberAtRest =  -camberAngleAtRest;
				suspensions[i + 0].mCamberAtMaxDroop = camberAngleAtMaxDroop;
				suspensions[i + 1].mCamberAtMaxDroop = -camberAngleAtMaxDroop;
				suspensions[i + 0].mCamberAtMaxCompression = camberAngleAtMaxCompression;
				suspensions[i + 1].mCamberAtMaxCompression = -camberAngleAtMaxCompression;
			}
		}

		//Set up the wheel geometry.
		physx::PxVec3 suspTravelDirections[PX_MAX_NB_WHEELS];
		physx::PxVec3 wheelCentreCMOffsets[PX_MAX_NB_WHEELS];
		physx::PxVec3 suspForceAppCMOffsets[PX_MAX_NB_WHEELS];
		physx::PxVec3 tireForceAppCMOffsets[PX_MAX_NB_WHEELS];
		{
			//Set the geometry data.
			for(uint32 i = 0; i < numWheels; i++)
			{
				//Vertical suspension travel.
				suspTravelDirections[i] = physx::PxVec3(0,-1,0);

				//Wheel center offset is offset from rigid body center of mass.
				wheelCentreCMOffsets[i] = wheelCenterActorOffsets[i] - chassisCMOffset;

				//Suspension force application point 0.3 metres below
				//rigid body center of mass.
				suspForceAppCMOffsets[i] = physx::PxVec3(wheelCentreCMOffsets[i].x, -0.3f, wheelCentreCMOffsets[i].z);

				//Tire force application point 0.3 metres below
				//rigid body center of mass.
				tireForceAppCMOffsets[i] = physx::PxVec3(wheelCentreCMOffsets[i].x, -0.3f, wheelCentreCMOffsets[i].z);
			}
		}

		//Set up the filter data of the raycast that will be issued by each suspension.
		physx::PxFilterData qryFilterData;
		qryFilterData.word0 = wheelRaycastGroup;
		qryFilterData.word1 = wheelRaycastMask;

		//Set the wheel, tire and suspension data.
		//Set the geometry data.
		//Set the query filter data
		for(uint32 i = 0; i < numWheels; i++)
		{
			wheelsSimData->setWheelData(i, wheels[i]);
			wheelsSimData->setTireData(i, tires[i]);
			wheelsSimData->setSuspensionData(i, suspensions[i]);
			wheelsSimData->setSuspTravelDirection(i, suspTravelDirections[i]);
			wheelsSimData->setWheelCentreOffset(i, wheelCentreCMOffsets[i]);
			wheelsSimData->setSuspForceAppPointOffset(i, suspForceAppCMOffsets[i]);
			wheelsSimData->setTireForceAppPointOffset(i, tireForceAppCMOffsets[i]);
			wheelsSimData->setSceneQueryFilterData(i, qryFilterData);
			wheelsSimData->setWheelShapeMapping(i, i);
		}
	}

	void PhysXVehicleInternal::SetupDriveSimData4W(physx::PxVehicleDriveSimData4W *driveSimData, physx::PxVehicleWheelsSimData* wheelsSimData)
	{
		//Diff
		physx::PxVehicleDifferential4WData diff;
		diff.mType = physx::PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;
		driveSimData->setDiffData(diff);

		//Engine
		physx::PxVehicleEngineData engine;
		engine.mPeakTorque = 700.0f;
		engine.mMaxOmega = 800.0f;//approx 8000 rpm
		driveSimData->setEngineData(engine);

		//Gears
		physx::PxVehicleGearsData gears;
		gears.mSwitchTime = 0.5f;
		driveSimData->setGearsData(gears);

		//Clutch
		physx::PxVehicleClutchData clutch;
		clutch.mStrength = 10.0f;
		driveSimData->setClutchData(clutch);

		//Ackermann steer accuracy
		physx::PxVehicleAckermannGeometryData ackermann;
		ackermann.mAccuracy = 1.0f;
		ackermann.mAxleSeparation = wheelsSimData->getWheelCentreOffset(physx::PxVehicleDrive4WWheelOrder::eREAR_LEFT).z - wheelsSimData->getWheelCentreOffset(physx::PxVehicleDrive4WWheelOrder::eFRONT_LEFT).z;
		ackermann.mFrontWidth = wheelsSimData->getWheelCentreOffset(physx::PxVehicleDrive4WWheelOrder::eFRONT_RIGHT).x - wheelsSimData->getWheelCentreOffset(physx::PxVehicleDrive4WWheelOrder::eFRONT_LEFT).x;
		ackermann.mRearWidth = wheelsSimData->getWheelCentreOffset(physx::PxVehicleDrive4WWheelOrder::eREAR_RIGHT).x - wheelsSimData->getWheelCentreOffset(physx::PxVehicleDrive4WWheelOrder::eREAR_LEFT).x;
		driveSimData->setAckermannGeometryData(ackermann);
	}

	void PhysXVehicleInternal::SetupVehicleActor(PhysXCompoundShape *compoundShape, const uint32 numWheels, const physx::PxVehicleChassisData& chassisData, const physx::PxFilterData& wheelSimFilterData, const physx::PxFilterData& chassisSimFilterData, physx::PxRigidDynamic *vehActor)
	{
		//Wheel and chassis query filter data.
		//Optional: cars don't drive on other cars.
		/*PxFilterData wheelQryFilterData;
		setupNonDrivableSurface(wheelQryFilterData);
		PxFilterData chassisQryFilterData;
		setupNonDrivableSurface(chassisQryFilterData);*/
		
		int counter = 0;
		for(PhysXShape *tempShape : compoundShape->_shapes)
		{
			physx::PxShape *physxShape = tempShape->GetPhysXShape();
			vehActor->attachShape(*physxShape);
			physxShape->setLocalPose(physx::PxTransform(physx::PxIdentity));
			
			if(counter < numWheels)
			{
				//physxShape->setQueryFilterData(wheelQryFilterData);
				physxShape->setSimulationFilterData(wheelSimFilterData);
			}
			else
			{
				//physxShape->setQueryFilterData(chassisQryFilterData);
				physxShape->setSimulationFilterData(chassisSimFilterData);
			}
			counter += 1;
		}

		vehActor->setMass(chassisData.mMass);
		vehActor->setMassSpaceInertiaTensor(chassisData.mMOI);
		vehActor->setCMassLocalPose(physx::PxTransform(chassisData.mCMOffset, physx::PxQuat(physx::PxIdentity)));
	}

	physx::PxVehicleDrivableSurfaceToTireFrictionPairs* PhysXVehicleInternal::CreateFrictionPairs(const physx::PxMaterial* defaultMaterial)
	{
		physx::PxVehicleDrivableSurfaceType surfaceTypes[1];
		surfaceTypes[0].mType = 0;

		const physx::PxMaterial* surfaceMaterials[1];
		surfaceMaterials[0] = defaultMaterial;

		physx::PxVehicleDrivableSurfaceToTireFrictionPairs* surfaceTirePairs = physx::PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(1, 1);

		surfaceTirePairs->setup(1, 1, &surfaceMaterials[0], surfaceTypes);

		for(uint32 i = 0; i < 1; i++)
		{
			for(uint32 j = 0; j < 1; j++)
			{
				surfaceTirePairs->setTypePairFriction(i, j, 1.0f);
			}
		}
		return surfaceTirePairs;
	}
}
