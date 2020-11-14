//
//  RNPhysXVehicle4WheelDrive.h
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSXVEHICLE4WHEELDRIVE_H_
#define __RAYNE_PHYSXVEHICLE4WHEELDRIVE_H_

#include "RNPhysX.h"
#include "RNPhysXCollisionObject.h"
#include "RNPhysXShape.h"

namespace physx
{
	class PxBatchQuery;
	class PxVehicleDrive4W;
	class PxRigidDynamic;
}

namespace RN
{
	class PhysXKinematicControllerCallback;
	class PhysXVehicle4WheelDrive : public PhysXCollisionObject
	{
	public:
		PXAPI PhysXVehicle4WheelDrive(PhysXCompoundShape *compoundShape, int8 wheelCount, uint32 chassisCollisionGroup, uint32 chassisCollisionMask, uint32 wheelCollisionGroup, uint32 wheelCollisionMask);
		PXAPI ~PhysXVehicle4WheelDrive() override;

		PXAPI void UpdatePosition() override;
		PXAPI void SetCollisionFilter(uint32 group, uint32 mask) override;
		
		PXAPI void SetWheelEntities(Array *wheelEntities);
		
		PXAPI void Update(float delta) override;
		
		PXAPI void SetAcceleration(float acceleration);
		PXAPI void SetBreak(float strength);
		PXAPI void SetSteering(float steer);
			
	private:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		
		PhysXCompoundShape *_shape;
		
		Array *_wheelEntities;
		int8 _wheelCount;
		
		physx::PxVehicleDrive4W *_vehicleDrive4W;
		physx::PxBatchQuery *_batchQuery;
		physx::PxRigidDynamic *_actor;
		void *_raycastQueryResults;
		void *_raycastHitBuffer;
			
		RNDeclareMetaAPI(PhysXVehicle4WheelDrive, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXVEHICLE4WHEELDRIVE_H_) */
