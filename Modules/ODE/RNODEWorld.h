//
//  RNODEWorld.h
//  Rayne-ODE
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ODEWORLD_H_
#define __RAYNE_ODEWORLD_H_

#include "RNODE.h"

struct dxWorld;
struct dxSpace;
struct dxJointGroup;
struct dxGeom;

namespace RN
{
	class ODECollisionObject;

	struct ODEContactInfo
	{
		SceneNode *node;
		Vector3 position;
		Vector3 normal;
		float distance;
	};

	class ODEWorld : public SceneAttachment
	{
	public:
		ODEAPI ODEWorld(const Vector3 &gravity = Vector3(0.0f, -9.81f, 0.0f));
		ODEAPI ~ODEWorld();

		ODEAPI void SetGravity(const Vector3 &gravity);

		ODEAPI void Update(float delta) override;
		ODEAPI void SetStepSize(double stepsize, int maxsteps);
		ODEAPI void SetSolverIterations(int iterations);

		ODEAPI ODEContactInfo CastRay(const Vector3 &from, const Vector3 &to);

		ODEAPI void InsertCollisionObject(ODECollisionObject *attachment);
		ODEAPI void RemoveCollisionObject(ODECollisionObject *attachment);

//		ODEAPI void InsertConstraint(BulletConstraint *constraint);

//		ODEAPI btDynamicsWorld *GetODEDynamicsWorld() { return _dynamicsWorld; }

		ODEAPI void SetPaused(bool paused);

		dxWorld *GetODEWorld() const { return _world; }
		dxSpace *GetODESpace() const { return _space; }
		static ODEWorld *GetSharedInstance() { return _sharedInstance; }

	private:
		static void SimulationStepTickCallback(void *data, dxGeom *object1, dxGeom *object2);
		static ODEWorld *_sharedInstance;

		dxWorld *_world;
		dxSpace *_space;
		dxJointGroup *_contactGroup;

		double _stepSize;
		int _maxSteps;
		bool _paused;

		std::unordered_set<ODECollisionObject *> _collisionObjects;

		RNDeclareMetaAPI(ODEWorld, ODEAPI)
	};
}


#endif /* __RAYNE_ODEWORLD_H_ */
