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
#include "RNAutoreleasePool.h"
#include "RNCamera.h"
#include "RNEntity.h"
#include "RNArray.h"
#include "RNRenderingResource.h"
#include "RNRenderingPipeline.h"
#include "RNPhysicsPipeline.h"

namespace RN
{
	class Kernel;
	class World : public UnconstructingSingleton<World>
	{
	friend class Entity;
	friend class Camera;
	friend class Kernel;
	public:
		RNAPI World();
		RNAPI virtual ~World();
		
		RNAPI virtual void Update(float delta);
		
		RNAPI PhysicsPipeline *Physics() const { return _physics; }
		
	private:
		void AddEntity(Entity *entity);
		void RemoveEntity(Entity *entity);
		
		void AddCamera(Camera *camera);
		void RemoveCamera(Camera *camera);
		
		void BeginUpdate(float delta);
		void FinishUpdate(float delta);
		
		Kernel *_kernel;
		
		std::vector<Camera *> _cameras;
		std::vector<Entity *> _entities;
		
		PhysicsPipeline *_physics;
		RenderingPipeline *_renderer;
		ReleasePool *_entityPool;
		
		PipelineSegment::TaskID _physicsTask;
		PipelineSegment::TaskID _renderingTask;
	};
}

#endif /* __RAYNE_WORLD_H__ */
