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
	friend class RigidBodyEntity;
	public:
		PhysicsPipeline();
		
		void SetGravity(const Vector3& gravity);
		
	private:
		enum
		{
			GravityChange = (1 << 0)
		};
		
		struct RemovedRigidBody
		{
			btCollisionShape *shape;
			btTriangleMesh *triangleMesh;
			btRigidBody *rigidbody;
		};
		
		virtual void WorkOnTask(TaskID task, float delta);
		virtual void WorkLoop();
		
		void ChangedRigidBody(RigidBodyEntity *entity);
		void RemoveRigidBody(RigidBodyEntity *entity);
		
		btBroadphaseInterface *_broadphase;
		btDefaultCollisionConfiguration *_collisionConfiguration;
		btCollisionDispatcher *_dispatcher;
		btSequentialImpulseConstraintSolver *_solver;
		btDynamicsWorld *_dynamicsWorld;
		
		std::unordered_set<RigidBodyEntity *> _changedRigidEntities;
		std::vector<RemovedRigidBody> _removedRigidEntities;
		
		Vector3 _gravity;
		uint32 _changes;
	};
}

#endif /* __RAYNE_PHYSICSPIPELINE_H__ */
