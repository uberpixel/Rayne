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
#include <unordered_set>

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

	class PhysXWorld : public SceneAttachment
	{
	public:
		PXAPI PhysXWorld(const Vector3 &gravity = Vector3(0.0f, -9.81f, 0.0f), String *pvdServerIP = nullptr);
		PXAPI ~PhysXWorld();

		PXAPI void SetGravity(const Vector3 &gravity);
		PXAPI Vector3 GetGravity();

		PXAPI void Update(float delta) override;
		PXAPI void SetSubsteps(uint8 substeps);
		PXAPI void SetPaused(bool paused);

		PXAPI PhysXContactInfo CastRay(const Vector3 &from, const Vector3 &to, uint32 filterMask = 0xffffffff);

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

		uint8 _substeps;
		bool _paused;

		PhysXSimulationCallback *_simulationCallback;

		RNDeclareMetaAPI(PhysXWorld, PXAPI)
	};
}


#endif /* __RAYNE_PHYSXWORLD_H_ */
