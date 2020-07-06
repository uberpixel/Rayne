//
//  RNBulletWorld.h
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BULLETWORLD_H_
#define __RAYNE_BULLETWORLD_H_

#include "RNBullet.h"

class btDynamicsWorld;
class btBroadphaseInterface;
class btCollisionConfiguration;
class btCollisionDispatcher;
class btConstraintSolver;
class btOverlappingPairCallback;

namespace RN
{
	class BulletCollisionObject;
	class BulletConstraint;

	struct BulletContactInfo
	{
        BulletContactInfo() : node(nullptr), distance(-1.0f) {}
        
		SceneNode *node;
		Vector3 position;
		Vector3 normal;
		float distance;
	};

	class BulletWorld : public SceneAttachment
	{
	public:
		BTAPI BulletWorld(const Vector3 &gravity = Vector3(0.0f, -9.81f, 0.0f));
		BTAPI ~BulletWorld();

		BTAPI void SetGravity(const Vector3 &gravity);

		BTAPI void Update(float delta) override;
		BTAPI void SetStepSize(double stepsize, int maxsteps);
		BTAPI void SetSolverIterations(int iterations);

		BTAPI BulletContactInfo CastRay(const Vector3 &from, const Vector3 &to);

		BTAPI void InsertCollisionObject(BulletCollisionObject *attachment);
		BTAPI void RemoveCollisionObject(BulletCollisionObject *attachment);

		BTAPI void InsertConstraint(BulletConstraint *constraint);

		BTAPI btDynamicsWorld *GetBulletDynamicsWorld() { return _dynamicsWorld; }

		BTAPI void SetPaused(bool paused);

	private:
		static void SimulationStepTickCallback(btDynamicsWorld *world, float timeStep);

        Lockable _lock;
        
		btDynamicsWorld *_dynamicsWorld;
		btBroadphaseInterface *_broadphase;
		btCollisionConfiguration *_collisionConfiguration;
		btCollisionDispatcher *_dispatcher;
		btConstraintSolver *_constraintSolver;
		btOverlappingPairCallback *_pairCallback;

		double _stepSize;
		int _maxSteps;
		bool _paused;

		std::unordered_set<BulletCollisionObject *> _collisionObjects;

		RNDeclareMetaAPI(BulletWorld, BTAPI)
	};
}


#endif /* __RAYNE_BULLETWORLD_H_ */
