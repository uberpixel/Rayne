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

#include "RNPhysXConstraint.h"
#include "RNPhysXDynamicBody.h"
#include "RNPhysXKinematicController.h"
#include "RNPhysXMaterial.h"
#include "RNPhysXShape.h"
#include "RNPhysXStaticBody.h"
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
} // namespace physx

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
		PXAPI std::vector<PhysXContactInfo> CastRayAll(const Vector3 &from, const Vector3 &to, uint32 filterGroup, uint32 filterMask, uint32 maxNumberOfOverlaps = 256);
		PXAPI PhysXContactInfo CastSweep(PhysXShape *shape, const Quaternion &rotation, const Vector3 &from, const Vector3 &to, float inflation = 0.0f, uint32 filterGroup = 0xffffffff, uint32 filterMask = 0xffffffff);
		PXAPI std::vector<PhysXContactInfo> CheckOverlap(PhysXShape *shape, const Vector3 &position, const Quaternion &rotation, float inflation = 0.0f, uint32 filterGroup = 0xffffffff, uint32 filterMask = 0xffffffff, uint32 maxNumberOfOverlaps = 256);
		PXAPI bool ComputePenetration(PhysXShape *shape0, const Vector3 &position0, const Quaternion &rotation0, PhysXShape *shape1, const Vector3 &position1, const Quaternion &rotation1, Vector3 &outDirection, float &outDepth);

		PXAPI physx::PxPhysics *GetPhysXInstance() const { return _physics; }
		PXAPI physx::PxCooking *GetPhysXCooking() const { return _cooking; }
		PXAPI physx::PxScene *GetPhysXScene() const { return _scene; }
		PXAPI physx::PxControllerManager *GetPhysXControllerManager() const { return _controllerManager; }

		static PhysXWorld *GetSharedInstance() { return _sharedInstance; }

		PXAPI void EnqueuePoseChange(PhysXCollisionObject *collisionObject);
		PXAPI void ClearPoseQueueSlot(size_t slot);

	private:
		void FlushQueuedPoseChanges();

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

		//Just making this big enough to hopefully never be a problem...
		AtomicRingBuffer<PhysXCollisionObject *, 50000> _pendingPoseChanges;

		RNDeclareMetaAPI(PhysXWorld, PXAPI)
	};
} // namespace RN


#endif /* __RAYNE_PHYSXWORLD_H_ */
