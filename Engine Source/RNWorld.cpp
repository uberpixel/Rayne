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

namespace RN
{
	World::World()
	{
		_kernel = Kernel::SharedInstance();
		_kernel->SetWorld(this);
		
		_renderer = _kernel->Renderer();
		_physics  = new PhysicsPipeline();
		
		_physicsTask   = kPipelineSegmentNullTask;
		_renderingTask = kPipelineSegmentNullTask;
	}
	
	World::~World()
	{
		_physics->WaitForTaskCompletion(_physicsTask);
		_renderer->WaitForTaskCompletion(_renderingTask);
		
		delete _physics;
		
		_kernel->SetWorld(0);
	}
	
	
	
	void World::Update(float delta)
	{}
	
	void World::BeginUpdate(float delta)
	{
		_physicsTask = _physics->BeginTask(delta);
		
		Update(delta);
		
		for(auto i=_transforms.begin(); i!=_transforms.end(); i++)
		{
			Transform *transform = *i;
			transform->Update(delta);
		}
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
						group->lights.AddObject((LightEntity *)entity);
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
	
	void World::FinishUpdate(float delta)
	{
		_physics->WaitForTaskCompletion(_physicsTask);
		
		for(auto i=_transforms.begin(); i!=_transforms.end(); i++)
		{
			Transform *transform = *i;
			transform->PostUpdate();
		}
		
		_renderer->WaitForTaskCompletion(_renderingTask);
		
		// Begin the new frame
		_renderer->PepareFrame();
		_renderingTask = _renderer->BeginTask(delta);
		
		for(auto i=_cameras.begin(); i!=_cameras.end(); i++)
		{
			Camera *camera = *i;
			
			RenderingGroup *group = new RenderingGroup(camera);
			
			for(auto j=_transforms.begin(); j!=_transforms.end(); j++)
			{
				Transform *transform = *j;
				VisitTransform(camera, transform, group);
			}
			
			_renderer->PushGroup(group);
		}
		
		_renderer->FinishFrame();
	}
	
	
	
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
