//
//  RNODEWorld.cpp
//  Rayne-ODE
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNODEWorld.h"
#include "RNODECollisionObject.h"

#include <ode/ode.h>

namespace RN
{
	RNDefineMeta(ODEWorld, SceneAttachment)

	ODEWorld *ODEWorld::_sharedInstance = nullptr;

	ODEWorld::ODEWorld(const Vector3 &gravity) : _maxSteps(50), _stepSize(1.0 / 120.0), _paused(false)
	{
		dInitODE();
		_world = dWorldCreate();
		_space = dHashSpaceCreate(0);
		dWorldSetGravity(_world, gravity.x, gravity.y, gravity.z);
//		dWorldSetCFM(_world, 1e-5);
		_contactGroup = dJointGroupCreate(0);

		_sharedInstance = this;
	}

	ODEWorld::~ODEWorld()
	{
		_sharedInstance = nullptr;

		dJointGroupDestroy(_contactGroup);
		dSpaceDestroy(_space);
		dWorldDestroy(_world);
		dCloseODE();
	}

	void ODEWorld::SimulationStepTickCallback(void *data, dxGeom *object1, dxGeom *object2)
	{
		ODEWorld *world = static_cast<ODEWorld*>(data);
		dxBody *body1 = dGeomGetBody(object1);
		dxBody *body2 = dGeomGetBody(object2);

		dContact contact;
		contact.surface.mode = 0;
		contact.surface.mu = 0.01f; //friction
		int numberOfContacts = dCollide(object1, object2, 1, &contact.geom, sizeof(dContact));
		if(numberOfContacts)
		{
			dxJoint *joint = dJointCreateContact(world->_world, world->_contactGroup, &contact);
			dJointAttach(joint, body1, body2);
		}
	}

	void ODEWorld::SetGravity(const Vector3 &gravity)
	{
		//TODO: Add lock!?
		dWorldSetGravity(_world, gravity.x, gravity.y, gravity.z);
	}

	void ODEWorld::SetStepSize(double stepsize, int maxsteps)
	{
		_stepSize = stepsize;
		_maxSteps = maxsteps;
	}

	void ODEWorld::SetPaused(bool paused)
	{
		_paused = paused;
	}

	void ODEWorld::SetSolverIterations(int iterations)
	{
//		_dynamicsWorld->getSolverInfo().m_numIterations = iterations;
	}

	void ODEWorld::Update(float delta)
	{
		if(_paused || delta < k::EpsilonFloat)
			return;

		dSpaceCollide(_space, this, &ODEWorld::SimulationStepTickCallback);
		dWorldStep(_world, delta);
		dJointGroupEmpty(_contactGroup);
	}



	ODEContactInfo ODEWorld::CastRay(const Vector3 &from, const Vector3 &to)
	{
/*		btVector3 btRayFrom = btVector3(from.x, from.y, from.z);
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
		}*/

		ODEContactInfo hit;
		return hit;
	}


	void ODEWorld::InsertCollisionObject(ODECollisionObject *attachment)
	{
		//TODO: Add lock!?
		auto iterator = _collisionObjects.find(attachment);
		if(iterator == _collisionObjects.end())
		{
			attachment->InsertIntoWorld(this);
			_collisionObjects.insert(attachment);
		}
	}

	void ODEWorld::RemoveCollisionObject(ODECollisionObject *attachment)
	{
		//TODO: Add lock!?
		auto iterator = _collisionObjects.find(attachment);
		if(iterator != _collisionObjects.end())
		{
			attachment->RemoveFromWorld(this);
			_collisionObjects.erase(attachment);
		}
	}

/*	void BulletWorld::InsertConstraint(BulletConstraint *constraint)
	{
		_dynamicsWorld->addConstraint(constraint->GetBulletConstraint());
	}*/
}
