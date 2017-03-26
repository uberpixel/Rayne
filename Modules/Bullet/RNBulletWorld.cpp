//
//  RNBulletWorld.cpp
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBulletWorld.h"
#include "RNBulletCollisionObject.h"

#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

namespace RN
{
	RNDefineMeta(BulletWorld, SceneAttachment)

	BulletWorld::BulletWorld(const Vector3 &gravity) : _maxSteps(10), _stepSize(1.0 / 60.0)
	{
		_pairCallback = new btGhostPairCallback();

		_broadphase = new btDbvtBroadphase();
		_broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(_pairCallback);

		_collisionConfiguration = new btDefaultCollisionConfiguration();
		_dispatcher = new btCollisionDispatcher(_collisionConfiguration);

		_constraintSolver = new btSequentialImpulseConstraintSolver();

		_dynamicsWorld = new btDiscreteDynamicsWorld(_dispatcher, _broadphase, _constraintSolver, _collisionConfiguration);
		_dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));

		_dynamicsWorld->setInternalTickCallback(&BulletWorld::SimulationStepTickCallback);
	}

	BulletWorld::~BulletWorld()
	{
		delete _dynamicsWorld;
		delete _constraintSolver;
		delete _dispatcher;
		delete _collisionConfiguration;
		delete _broadphase;
		delete _pairCallback;
	}

	void BulletWorld::SimulationStepTickCallback(btDynamicsWorld *world, float timeStep)
	{
		int numManifolds = world->getDispatcher()->getNumManifolds();
		for(int i = 0; i<numManifolds; i++)
		{
			btPersistentManifold *contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
			BulletCollisionObject *objectA = static_cast<BulletCollisionObject *>(contactManifold->getBody0()->getUserPointer());
			BulletCollisionObject *objectB = static_cast<BulletCollisionObject *>(contactManifold->getBody1()->getUserPointer());

			if(objectA->_callback)
				objectA->_callback(objectB);

			if(objectB->_callback)
				objectB->_callback(objectA);
		}
	}

	void BulletWorld::SetGravity(const Vector3 &gravity)
	{
		//TODO: Add lock!?
		_dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
	}

	void BulletWorld::SetStepSize(double stepsize, int maxsteps)
	{
		_stepSize = stepsize;
		_maxSteps = maxsteps;
	}

	void BulletWorld::Update(float delta)
	{
		_dynamicsWorld->stepSimulation(delta, _maxSteps, _stepSize);
	}



	BulletContactInfo BulletWorld::CastRay(const Vector3 &from, const Vector3 &to)
	{
		btVector3 btRayFrom = btVector3(from.x, from.y, from.z);
		btVector3 btRayTo = btVector3(to.x, to.y, to.z);

		btCollisionWorld::ClosestRayResultCallback rayCallback(btRayFrom, btRayTo);

		Lock();
		_dynamicsWorld->rayTest(btRayFrom, btRayTo, rayCallback);
		Unlock();

		BulletContactInfo hit;

		if(rayCallback.hasHit())
		{
			BulletCollisionObject *body = reinterpret_cast<BulletCollisionObject *>(rayCallback.m_collisionObject->getUserPointer());

			hit.node = body->GetParent();
			hit.position = Vector3(rayCallback.m_hitPointWorld.x(), rayCallback.m_hitPointWorld.y(), rayCallback.m_hitPointWorld.z());
			hit.normal = Vector3(rayCallback.m_hitNormalWorld.x(), rayCallback.m_hitNormalWorld.y(), rayCallback.m_hitNormalWorld.z());
			hit.distance = hit.position.GetDistance(from);
		}

		return hit;
	}


	void BulletWorld::InsertCollisionObject(BulletCollisionObject *attachment)
	{
		//TODO: Add lock!?
		auto iterator = _collisionObjects.find(attachment);
		if(iterator == _collisionObjects.end())
		{
			attachment->InsertIntoWorld(this);
			_collisionObjects.insert(attachment);
		}
	}

	void BulletWorld::RemoveCollisionObject(BulletCollisionObject *attachment)
	{
		//TODO: Add lock!?
		auto iterator = _collisionObjects.find(attachment);
		if(iterator != _collisionObjects.end())
		{
			attachment->RemoveFromWorld(this);
			_collisionObjects.erase(attachment);
		}
	}
}
