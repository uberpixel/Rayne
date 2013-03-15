//
//  RNWorld.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorld.h"
#include "RNKernel.h"
#include "RNTransform.h"
#include "RNEntity.h"
#include "RNLightEntity.h"
#include "RNAutoreleasePool.h"
#include "RNThreadPool.h"

namespace RN
{
	World::World()
	{
		_kernel = Kernel::SharedInstance();
		_kernel->SetWorld(this);
		
		_renderer = _kernel->Renderer();
		
		_physicsTask   = kPipelineSegmentNullTask;
		_renderingTask = kPipelineSegmentNullTask;
	}
	
	World::~World()
	{
		_renderer->WaitForTaskCompletion(_renderingTask);
		_kernel->SetWorld(0);
	}
	
	
	
	void World::Update(float delta)
	{}
	
	void World::BeginUpdate(float delta)
	{
		Update(delta);
		
		auto pool = ThreadCoordinator::SharedInstance()->GlobalPool();
		
		for(auto i=_transforms.begin(); i!=_transforms.end(); i++)
		{
			Transform *transform = *i;
			
			pool->AddTask([transform, delta]() {
				//transform->Update(delta);
				
				printf("Transform: %p\n", transform);
			});
		}
		
		printf("Waiting for tasks to complete...\n");
		pool->WaitForTasksToComplete();
		printf("Done..\n");
	}
	
	void World::VisitTransform(Camera *camera, Transform *transform, RenderingGroup *group)
	{
		if(transform->Type() == Transform::TransformTypeEntity)
		{
			Entity *entity = (Entity *)transform;
			
			if(entity->IsVisibleInCamera(camera))
			{
				if(entity->Model())
					group->entities.AddObject(entity);
				
				switch(entity->Type())
				{
					case Entity::TypeObject:
						break;
						
					case Entity::TypeLight:
						LightEntity *light = (LightEntity *)entity;
						if(light->LightType() == LightEntity::TypePointLight)
							group->pointLights.AddObject(light);
						if(light->LightType() == LightEntity::TypeSpotLight)
							group->spotLights.AddObject(light);
						if(light->LightType() == LightEntity::TypeDirectionalLight)
							group->directionalLights.AddObject(light);
						break;
				}
			}
		}
		
		for(machine_uint i=0; i<transform->Childs(); i++)
		{
			Transform *child = transform->ChildAtIndex(i);
			VisitTransform(camera, child, group);
		}
	}
	
		/*static float renderingTime = 0.0f;
		static float totalTime = 0.0f;
		static int count = 0;
		
		renderingTime += _renderer->UsedTime();
		totalTime += delta;
		count ++;
		
		if(totalTime >= 1.0f)
		{
			float average = renderingTime / count;
			
			printf("Drew %i frames in the last %f seconds. Average frame time: %f\n", count, totalTime, average);
			
			renderingTime = totalTime = 0.0f;
			count = 0;
		}*/
		
		// Begin the new frame

	
	
	
	void World::AddTransform(Transform *transform)
	{
		if(!transform)
			return;
		
		if(_transforms.find(transform) == _transforms.end())
		{
			_transforms.insert(transform);
				
			if(transform->Type() == Transform::TransformTypeCamera)
			{
				Camera *camera = (Camera *)transform;
				_cameras.push_back(camera);
			}
		}
	}
	
	void World::RemoveTransform(Transform *transform)
	{
		if(!transform)
			return;
		
		auto iterator = _transforms.find(transform);
		if(iterator != _transforms.end())
		{
			_transforms.erase(iterator);
			
			if(transform->Type() == Transform::TransformTypeCamera)
			{
				_cameras.erase(std::remove(_cameras.begin(), _cameras.end(), transform), _cameras.end());
			}
		}
	}
}
