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
		_gravity = Vector3(0.0, -9.81, 0.0);
		_changes = GravityChange;
		
		std::thread thread = std::thread(&PhysicsPipeline::WorkLoop, this);
		thread.detach();
	}
	
	void PhysicsPipeline::WorkOnTask(TaskID task, float delta)
	{
		// Remove the accumulated dead bodies
		for(auto i=_removedRigidEntities.begin(); i!=_removedRigidEntities.end(); i++)
		{
			_dynamicsWorld->removeRigidBody(i->rigidbody);
			
			delete i->rigidbody;
			delete i->shape;
			
			if(i->triangleMesh)
				delete i->triangleMesh;
		}
		
		_removedRigidEntities.clear();
		
		// Update rigid bodies
		for(auto i=_changedRigidEntities.begin(); i!=_changedRigidEntities.end(); i++)
		{
			RigidBodyEntity *entity = *i;
			entity->UpdateRigidBody(_dynamicsWorld);
		}
		
		_changedRigidEntities.clear();
		
		// Apply accumulated changes to the world
		if(_changes & GravityChange)
		{
			_dynamicsWorld->setGravity(btVector3(_gravity.x, _gravity.y, _gravity.z));
			_changes &= ~GravityChange;
		}
		
		// Simulate the world
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
	
	
	void PhysicsPipeline::ChangedRigidBody(RigidBodyEntity *entity)
	{
		_changedRigidEntities.insert(entity);
	}
	
	void PhysicsPipeline::RemoveRigidBody(RigidBodyEntity *entity)
	{
		_changedRigidEntities.erase(entity);
		
		RemovedRigidBody body;
		body.shape = entity->_shape;
		body.triangleMesh = entity->_triangleMesh;
		body.rigidbody = entity->_rigidbody;
		
		_removedRigidEntities.push_back(body);
	}
	
	
	
	void PhysicsPipeline::SetGravity(const Vector3& gravity)
	{
		_gravity = gravity;
		_changes |= GravityChange;
	}
}
