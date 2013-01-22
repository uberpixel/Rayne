//
//  RNWorld.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WORLD_H__
#define __RAYNE_WORLD_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNCamera.h"
#include "RNEntity.h"
#include "RNArray.h"
#include "RNRenderingResource.h"
#include "RNRenderingPipeline.h"
#include "RNPhysicsPipeline.h"

namespace RN
{
	class Kernel;
	class World : public Object, public UnconstructingSingleton<World>
	{
	friend class Entity;
	friend class Kernel;
	public:
		RNAPI World(Kernel *kernel);
		RNAPI virtual ~World();
		
		RNAPI virtual void Update(float delta);
		
	private:
		void AddEntity(Entity *entity);
		void RemoveEntity(Entity *entity);
		
		void BeginUpdate(float delta);
		void FinishUpdate();
		
		Kernel *_kernel;
		ObjectArray *_cameras;
		std::vector<Entity *> _entities;
		
		PhysicsPipeline *_physics;
		RenderingPipeline *_renderer;
		
		PipelineSegment::TaskID _physicsTask;
		PipelineSegment::TaskID _renderingTask;
		
		void CreateTestMesh();
	};
}

#endif /* __RAYNE_WORLD_H__ */
