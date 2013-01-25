//
//  RNPhysicsPipeline.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSICSPIPELINE_H__
#define __RAYNE_PHYSICSPIPELINE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNPipelineSegment.h"
#include "RNBullet.h"
#include "RNRigidBodyEntity.h"

namespace RN
{
	class PhysicsPipeline : public PipelineSegment
	{
	public:
		PhysicsPipeline();
		
		void AddRigidBody(RigidBodyEntity *entity);
		void RemoveRigidBody(RigidBodyEntity *entity);
		void SetGravity(Vector3 gravity);
		
	private:
		virtual void WorkOnTask(TaskID task, float delta);
		void WorkLoop();
		
		btBroadphaseInterface *_broadphase;
		btDefaultCollisionConfiguration *_collisionConfiguration;
		btCollisionDispatcher *_dispatcher;
		btSequentialImpulseConstraintSolver *_solver;
		btDynamicsWorld *_dynamicsWorld;
		
		std::vector<RigidBodyEntity*> _addedRigidEntities;
		std::vector<RigidBodyEntity*> _removedRigidEntities;
		
		Vector3 _gravity;
	};
}

#endif /* __RAYNE_PHYSICSPIPELINE_H__ */
