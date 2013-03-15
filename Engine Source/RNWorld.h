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
#include "RNRenderingResource.h"
#include "RNRenderingPipeline.h"
#include "RNCamera.h"

namespace RN
{
	class Kernel;
	class Transform;
	class RetainPool;
	
	class World : public NonConstructingSingleton<World>
	{
	friend class Transform;
	friend class Kernel;
	public:
		RNAPI World();
		RNAPI virtual ~World();
		
		RNAPI void AddTransform(Transform *transform);
		RNAPI void RemoveTransform(Transform *transform);
		
		RNAPI virtual void Update(float delta);
		
	private:
		void BeginUpdate(float delta);		
		void VisitTransform(Camera *camera, Transform *transform, RenderingGroup *group);
		
		Kernel *_kernel;
		std::unordered_set<Transform *> _transforms;
		std::vector<Camera *> _cameras;
		
		RenderingPipeline *_renderer;
		
		PipelineSegment::TaskID _physicsTask;
		PipelineSegment::TaskID _renderingTask;
	};
}

#endif /* __RAYNE_WORLD_H__ */
