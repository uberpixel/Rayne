//
//  RNPhysicsPipeline.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysicsPipeline.h"
#include <btBulletDynamicsCommon.h>

namespace RN
{
	PhysicsPipeline::PhysicsPipeline()
	{
		std::thread thread = std::thread(&PhysicsPipeline::WorkLoop, this);
		thread.detach();
	}
	
	void PhysicsPipeline::WorkOnTask(TaskID task, float delta)
	{
		for(auto i=_addedRigidEntities.begin(); i!=_addedRigidEntities.end(); i++)
		{
			RigidBodyEntity *entity = *i;
			entity->InitializeRigidBody(_dynamicsWorld);
		}
		_addedRigidEntities.clear();
		
		_dynamicsWorld->stepSimulation(delta, 10);
	}
	
	void PhysicsPipeline::WorkLoop()
	{

		btBroadphaseInterface *_broadphase = new btDbvtBroadphase();
		
		// Set up the collision configuration and dispatcher
		btDefaultCollisionConfiguration *_collisionConfiguration = new btDefaultCollisionConfiguration();
		btCollisionDispatcher *_dispatcher = new btCollisionDispatcher(_collisionConfiguration);
		
		// The actual physics solver
		btSequentialImpulseConstraintSolver *_solver = new btSequentialImpulseConstraintSolver;
		
		// The world.
		_dynamicsWorld = new btDiscreteDynamicsWorld(_dispatcher, _broadphase, _solver, _collisionConfiguration);
		_dynamicsWorld->setGravity(btVector3(0, -9.81, 0));
		
		while(!ShouldStop())
		{
			WaitForWork();
		}
		
		delete _dynamicsWorld;
		delete _solver;
		delete _dispatcher;
		delete _collisionConfiguration;
		delete _broadphase;
		
		Thread::CurrentThread()->Exit();
		DidStop();
	}
	
	void PhysicsPipeline::AddRigidBody(RigidBodyEntity *entity)
	{
		_addedRigidEntities.push_back(entity);
	}
}
