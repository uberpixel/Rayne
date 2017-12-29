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
#include "RNPhysXRigidBody.h"

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
	struct BulletContactInfo
	{
		SceneNode *node;
		Vector3 position;
		Vector3 normal;
		float distance;
	};

	class PhysXWorld : public SceneAttachment
	{
	public:
		PXAPI PhysXWorld(const Vector3 &gravity = Vector3(0.0f, -9.81f, 0.0f), bool debug = false);
		PXAPI ~PhysXWorld();

		PXAPI void SetGravity(const Vector3 &gravity);

		PXAPI void Update(float delta) override;
		PXAPI void SetStepSize(double stepsize);
		PXAPI void SetPaused(bool paused);

		PXAPI BulletContactInfo CastRay(const Vector3 &from, const Vector3 &to);

		PXAPI void InsertCollisionObject(PhysXCollisionObject *attachment);
		PXAPI void RemoveCollisionObject(PhysXCollisionObject *attachment);

//		PXAPI void InsertConstraint(BulletConstraint *constraint);

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

		double _remainingTime;
		double _stepSize;
		bool _paused;

		std::unordered_set<PhysXCollisionObject *> _collisionObjects;

		RNDeclareMetaAPI(PhysXWorld, PXAPI)
	};
}


#endif /* __RAYNE_PHYSXWORLD_H_ */
