//
//  RNPhysXWorld.h
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSXWORLD_H_
#define __RAYNE_PHYSXWORLD_H_

#include "RNPhysX.h"

#include "RNPhysXMaterial.h"
#include "RNPhysXShape.h"
#include "RNPhysXDynamicBody.h"
#include "RNPhysXStaticBody.h"
#include "RNPhysXConstraint.h"
#include "RNPhysXKinematicController.h"
#include "RNPhysXVehicle4WheelDrive.h"

namespace physx
{
	class PxFoundation;
	class PxPvd;
	class PxPhysics;
	class PxCooking;
	class PxScene;
	class PxDefaultCpuDispatcher;
	class PxControllerManager;
}

namespace RN
{
	class PhysXSimulationCallback;
	class PhysXKinematicControllerCallback;

	class PhysXWorld : public SceneAttachment
	{
	public:
		PXAPI PhysXWorld(const Vector3 &gravity = Vector3(0.0f, -9.81f, 0.0f), String *pvdServerIP = nullptr);
		PXAPI ~PhysXWorld();

		PXAPI void SetGravity(const Vector3 &gravity);
		PXAPI Vector3 GetGravity();
		
		PXAPI void InitializeVehicles();

		PXAPI void Update(float delta) final;
		PXAPI void WillUpdate(float delta) final;
		PXAPI void SetSubsteps(uint8 substeps);
		PXAPI void SetPaused(bool paused);

		PXAPI PhysXContactInfo CastRay(const Vector3 &from, const Vector3 &to, uint32 filterGroup = 0xffffffff, uint32 filterMask = 0xffffffff);
		PXAPI PhysXContactInfo CastSweep(PhysXShape *shape, const Quaternion &rotation, const Vector3 &from, const Vector3 &to, float inflation = 0.0f, uint32 filterGroup = 0xffffffff, uint32 filterMask = 0xffffffff);
		PXAPI std::vector<PhysXContactInfo> CheckOverlap(PhysXShape *shape, const Vector3 &position, const Quaternion &rotation, float inflation = 0.0f, uint32 filterGroup = 0xffffffff, uint32 filterMask = 0xffffffff, uint32 maxNumberOfOverlaps = 256);

		PXAPI physx::PxPhysics *GetPhysXInstance() const { return _physics; }
		PXAPI physx::PxCooking *GetPhysXCooking() const { return _cooking; }
		PXAPI physx::PxScene *GetPhysXScene() const { return _scene; }
		PXAPI physx::PxControllerManager *GetPhysXControllerManager() const { return _controllerManager; }

		static PhysXWorld *GetSharedInstance() { return _sharedInstance; }

	private:
		static PhysXWorld *_sharedInstance;

		physx::PxFoundation *_foundation;
		physx::PxPvd *_pvd;
		physx::PxPhysics *_physics;
		physx::PxCooking *_cooking;
		physx::PxScene *_scene;
		physx::PxDefaultCpuDispatcher *_dispatcher;
		physx::PxControllerManager *_controllerManager;
		
		bool _hasVehicles;
		bool _isSimulating;

		uint8 _substeps;
		bool _paused;

		PhysXSimulationCallback *_simulationCallback;
		PhysXKinematicControllerCallback *_controllerManagerFilterCallback;

		RNDeclareMetaAPI(PhysXWorld, PXAPI)
	};
}


#endif /* __RAYNE_PHYSXWORLD_H_ */
