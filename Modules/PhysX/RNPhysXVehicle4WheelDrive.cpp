//
//  RNPhysXVehicle4WheelDrive.cpp
//  Rayne-PhysX
//
//  Copyright 2020 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#include "RNPhysXVehicle4WheelDrive.h"
#include "RNPhysXWorld.h"
#include "RNPhysXInternals.h"

#include "PxPhysicsAPI.h"

namespace RN
{
	RNDefineMeta(PhysXVehicle4WheelDrive, PhysXCollisionObject)

/*	physx::PxVehicleKeySmoothingData gKeySmoothingData=
	{
		{
			3.0f,    //rise rate eANALOG_INPUT_ACCEL
			3.0f,    //rise rate eANALOG_INPUT_BRAKE
			10.0f,    //rise rate eANALOG_INPUT_HANDBRAKE
			2.5f,    //rise rate eANALOG_INPUT_STEER_LEFT
			2.5f,    //rise rate eANALOG_INPUT_STEER_RIGHT
		},
		{
			5.0f,    //fall rate eANALOG_INPUT__ACCEL
			5.0f,    //fall rate eANALOG_INPUT__BRAKE
			10.0f,    //fall rate eANALOG_INPUT__HANDBRAKE
			5.0f,    //fall rate eANALOG_INPUT_STEER_LEFT
			5.0f    //fall rate eANALOG_INPUT_STEER_RIGHT
		}
	};

	float gSteerVsForwardSpeedData[2*8]=
	{
		0.0f,        0.75f,
		5.0f,        0.75f,
		30.0f,        0.125f,
		120.0f,        0.1f,
		PX_MAX_F32, PX_MAX_F32,
		PX_MAX_F32, PX_MAX_F32,
		PX_MAX_F32, PX_MAX_F32,
		PX_MAX_F32, PX_MAX_F32
	};
	physx::PxFixedSizeLookupTable<8> gSteerVsForwardSpeedTable(gSteerVsForwardSpeedData,4);*/
		
	PhysXVehicle4WheelDrive::PhysXVehicle4WheelDrive(PhysXCompoundShape *compoundShape, int8 wheelCount, uint32 chassisCollisionGroup, uint32 chassisCollisionMask, uint32 wheelCollisionGroup, uint32 wheelCollisionMask) : _shape(compoundShape->Retain()), _wheelEntities(nullptr), _wheelCount(wheelCount)
	{
		//The first wheelCount objects in the compound shape need to be the tires!
		
		
		physx::PxVehicleChassisData chassisData;
		chassisData.mMass = 1400.0f;
		
		RN::Vector3 chassisDimensions(2.0f, 1.0f, 4.0f);
		
		//The origin is at the center of the chassis mesh.
		//Set the center of mass a bit lower
		chassisData.mCMOffset = physx::PxVec3(0.0f, -0.5f, 0.0f);

		//Now compute the chassis mass and moment of inertia.
		//Use the moment of inertia of a cuboid as an approximate value for the chassis moi.
		chassisData.mMOI = physx::PxVec3((chassisDimensions.y * chassisDimensions.y + chassisDimensions.z * chassisDimensions.z) * chassisData.mMass / 12.0f, (chassisDimensions.x * chassisDimensions.x + chassisDimensions.z * chassisDimensions.z) * chassisData.mMass / 12.0f, (chassisDimensions.x * chassisDimensions.x + chassisDimensions.y * chassisDimensions.y) * chassisData.mMass / 12.0f);
		//A bit of tweaking here.  The car will have more responsive turning if we reduce the
		//y-component of the chassis moment of inertia.
		chassisData.mMOI.y *= 0.6f;
		
		PhysXWorld *world = PhysXWorld::GetSharedInstance();
		physx::PxPhysics *physics = PhysXWorld::GetSharedInstance()->GetPhysXInstance();
		
		physx::PxVec3 *wheelCenterActorOffsets = new physx::PxVec3[_wheelCount];
		for(int i = 0; i < _wheelCount; i++)
		{
			RN::Vector3 wheelPosition = compoundShape->GetPosition(i);
			wheelCenterActorOffsets[i] = physx::PxVec3(wheelPosition.x, wheelPosition.y, wheelPosition.z);
		}
		physx::PxVehicleWheelsSimData *wheelsSimData = physx::PxVehicleWheelsSimData::allocate(_wheelCount);
		PhysXVehicleInternal::SetupWheelsSimulationData(25.0f, compoundShape, _wheelCount, wheelCenterActorOffsets, chassisData.mCMOffset, chassisData.mMass, wheelsSimData, wheelCollisionGroup, wheelCollisionMask);

		physx::PxVehicleDriveSimData4W driveSimData;
		PhysXVehicleInternal::SetupDriveSimData4W(&driveSimData, wheelsSimData);
		
		physx::PxFilterData wheelSimFilterData;
		wheelSimFilterData.word0 = 0;
		wheelSimFilterData.word1 = 0;
		
		physx::PxFilterData simFilterData;
		simFilterData.word0 = chassisCollisionGroup;
		simFilterData.word1 = chassisCollisionMask;

		_actor = physics->createRigidDynamic(physx::PxTransform(physx::PxIdentity));
		PhysXVehicleInternal::SetupVehicleActor(compoundShape, _wheelCount, chassisData, wheelSimFilterData, simFilterData, _actor);
		world->GetPhysXScene()->addActor(*_actor);
		_actor->userData = this;

		_vehicleDrive4W = physx::PxVehicleDrive4W::allocate(_wheelCount);
		_vehicleDrive4W->setup(physics, _actor, *wheelsSimData, driveSimData, _wheelCount - 4);
		wheelsSimData->free();
		
		_raycastQueryResults = static_cast<void*>(new physx::PxRaycastQueryResult[_wheelCount]);
		_raycastHitBuffer = static_cast<void*>(new physx::PxRaycastHit[_wheelCount]);
		physx::PxBatchQueryDesc sqDesc(_wheelCount, 0, 0);
		sqDesc.queryMemory.userRaycastResultBuffer = static_cast<physx::PxRaycastQueryResult*>(_raycastQueryResults);
		sqDesc.queryMemory.userRaycastTouchBuffer = static_cast<physx::PxRaycastHit*>(_raycastHitBuffer);
		sqDesc.queryMemory.raycastTouchBufferSize = _wheelCount;
		sqDesc.preFilterShader = PhysXCallback::VehicleQueryPreFilter;
		_batchQuery = world->GetPhysXScene()->createBatchQuery(sqDesc);
		
		_vehicleDrive4W->mDriveDynData.setUseAutoGears(true);
	}
	
	PhysXVehicle4WheelDrive::~PhysXVehicle4WheelDrive()
	{
		delete [] static_cast<physx::PxRaycastQueryResult*>(_raycastQueryResults);
		delete [] static_cast<physx::PxRaycastHit*>(_raycastHitBuffer);
		
		SafeRelease(_wheelEntities);
	}

	void PhysXVehicle4WheelDrive::Update(float delta)
	{
		if(delta > 1.0f/20.0f) return;
		
		physx::PxVehicleWheels *vehicles[1] = {_vehicleDrive4W};
		PxVehicleSuspensionRaycasts(_batchQuery, 1, vehicles, _wheelCount, static_cast<physx::PxRaycastQueryResult*>(_raycastQueryResults));
		
		Vector3 worldGravity = PhysXWorld::GetSharedInstance()->GetGravity();
		physx::PxVec3 gravity(worldGravity.x, worldGravity.y, worldGravity.z);
		
		physx::PxVehicleDrivableSurfaceToTireFrictionPairs *frictionPairs = PhysXVehicleInternal::CreateFrictionPairs(nullptr);
		
		PxVehicleUpdates(delta, gravity, *frictionPairs, 1, vehicles, nullptr, nullptr);
	}

	void PhysXVehicle4WheelDrive::SetAcceleration(float acceleration)
	{
/*		physx::PxVehicleDrive4WRawInputData rawInputData;
		rawInputData.setDigitalAccel(true);
		PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(gKeySmoothingData, gSteerVsForwardSpeedTable, rawInputData, 1.0f/60.0f, false, *_vehicleDrive4W);*/
		
		_vehicleDrive4W->mDriveDynData.setAnalogInput(physx::PxVehicleDrive4WControl::eANALOG_INPUT_ACCEL, acceleration);
	}

	void PhysXVehicle4WheelDrive::SetBreak(float strength)
	{
		_vehicleDrive4W->mDriveDynData.setAnalogInput(physx::PxVehicleDrive4WControl::eANALOG_INPUT_BRAKE, strength);
	}

	void PhysXVehicle4WheelDrive::SetSteering(float steer)
	{
		if(steer < 0.0f)
		{
			_vehicleDrive4W->mDriveDynData.setAnalogInput(physx::PxVehicleDrive4WControl::eANALOG_INPUT_STEER_LEFT, 0.0f);
			_vehicleDrive4W->mDriveDynData.setAnalogInput(physx::PxVehicleDrive4WControl::eANALOG_INPUT_STEER_RIGHT, -steer);
		}
		else
		{
			_vehicleDrive4W->mDriveDynData.setAnalogInput(physx::PxVehicleDrive4WControl::eANALOG_INPUT_STEER_RIGHT, 0.0f);
			_vehicleDrive4W->mDriveDynData.setAnalogInput(physx::PxVehicleDrive4WControl::eANALOG_INPUT_STEER_LEFT, steer);
		}
	}

	void PhysXVehicle4WheelDrive::SetGear(uint32 gear)
	{
		_vehicleDrive4W->mDriveDynData.setTargetGear(gear);
	}

	void PhysXVehicle4WheelDrive::SetCollisionFilter(uint32 group, uint32 mask)
	{
		PhysXCollisionObject::SetCollisionFilter(group, mask);

		physx::PxShape *shape;
		//_controller->getActor()->getShapes(&shape, 1);

		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		shape->setSimulationFilterData(filterData);
		shape->setQueryFilterData(filterData);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);

//		_controller->invalidateCache();
	}

	void PhysXVehicle4WheelDrive::SetWheelEntities(Array *wheelEntities)
	{
		RN_ASSERT(!wheelEntities || _wheelCount == wheelEntities->GetCount(), "Need to set same number of wheel entities as there have been wheels configured!");
		SafeRelease(_wheelEntities);
		_wheelEntities = SafeRetain(wheelEntities);
	}

	float PhysXVehicle4WheelDrive::GetCurrentSpeed()
	{
		float wheelRadius = 0.35f;
		float averageSpeed = 0.0f;
		for(int i = 0; i < _vehicleDrive4W->mWheelsDynData.getNbWheelRotationSpeed(); i++)
		{
			averageSpeed += _vehicleDrive4W->mWheelsDynData.getWheelRotationSpeed(i);
		}
		return averageSpeed * wheelRadius / _wheelCount;
	}

	float PhysXVehicle4WheelDrive::GetCurrentRPM()
	{
		return 60.0f * _vehicleDrive4W->mDriveDynData.getEngineRotationSpeed() / (2.0f * k::Pi);
	}

	uint32 PhysXVehicle4WheelDrive::GetCurrentGear()
	{
		return _vehicleDrive4W->mDriveDynData.getCurrentGear();
	}

	void PhysXVehicle4WheelDrive::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		PhysXCollisionObject::DidUpdate(changeSet);
		
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			RN::Vector3 positionOffset = GetWorldRotation().GetRotatedVector(_positionOffset);
			Vector3 position = GetWorldPosition() - positionOffset;
			Quaternion rotation = GetWorldRotation() * _rotationOffset;
			_actor->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent())
			{
				RN::Vector3 positionOffset = GetWorldRotation().GetRotatedVector(_positionOffset);
				Vector3 position = GetWorldPosition() - positionOffset;
				Quaternion rotation = GetWorldRotation() * _rotationOffset;
				_actor->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
			}

			_owner = GetParent();
		}
	}

	void PhysXVehicle4WheelDrive::UpdatePosition()
	{
		if(!_owner)
		{
			return;
		}

		const physx::PxTransform &transform = _actor->getGlobalPose();
		RN::Quaternion rotation = Quaternion(transform.q.x, transform.q.y, transform.q.z, transform.q.w) * _rotationOffset.GetConjugated();
		RN::Vector3 positionOffset = rotation.GetRotatedVector(_positionOffset);
		SetWorldPosition(Vector3(transform.p.x, transform.p.y, transform.p.z) + positionOffset);
		SetWorldRotation(rotation);
		
		if(_wheelEntities)
		{
			for(int i = 0; i < _wheelCount; i++)
			{
				PhysXShape *shape = _shape->GetShape(i);
				const physx::PxTransform &wheelPose = shape->GetPhysXShape()->getLocalPose();
				Entity *wheelEntity = _wheelEntities->GetObjectAtIndex<Entity>(i);
				
				wheelEntity->SetWorldPosition(GetWorldPosition() + GetWorldRotation().GetRotatedVector(Vector3(wheelPose.p.x, wheelPose.p.y, wheelPose.p.z)));
				wheelEntity->SetWorldRotation(GetWorldRotation() * Quaternion(wheelPose.q.x, wheelPose.q.y, wheelPose.q.z, wheelPose.q.w));
			}
		}
	}
}
