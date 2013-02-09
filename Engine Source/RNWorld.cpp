//
//  RNWorld.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorld.h"
#include "RNKernel.h"
#include "RNLightEntity.h"

namespace RN
{
	World::World()
	{
		_kernel = Kernel::SharedInstance();
		_kernel->SetWorld(this);
		
		_renderer = _kernel->Renderer();
		_physics  = new PhysicsPipeline();
		
		_physicsTask = kPipelineSegmentNullTask;
		_renderingTask = kPipelineSegmentNullTask;
	}
	
	World::~World()
	{
		delete _physics;
		_kernel->SetWorld(0);
	}
	
	
	
	void World::Update(float delta)
	{
	}
	
	void World::BeginUpdate(float delta)
	{
		_physicsTask = _physics->BeginTask(delta);
		
		Update(delta);
		
		for(auto i=_entities.begin(); i!=_entities.end(); i++)
		{
			Entity *entity = *i;
			entity->Update(delta);
		}
		
		for(auto i=_cameras.begin(); i!=_cameras.end(); i++)
		{
			Camera *camera = *i;
			camera->Update(delta);
		}
	}
	
	void World::FinishUpdate(float delta)
	{
		_physics->WaitForTaskCompletion(_physicsTask);
		
		for(auto i=_entities.begin(); i!=_entities.end(); i++)
		{
			Entity *entity = *i;
			entity->PostUpdate();
		}
		
		_renderer->WaitForTaskCompletion(_renderingTask);
		
		_renderer->PepareFrame();
		_renderingTask = _renderer->BeginTask(delta);
		
		for(auto i=_cameras.begin(); i!=_cameras.end(); i++)
		{
			Camera *camera = *i;
			for(Camera *cam = camera; cam != 0; cam = cam->Stage())
			{
				cam->UpdateFrustum();
				cam->SynchronizePast();
			}
			
			RenderingGroup group;
			group.camera = camera;
			
			for(auto j=_entities.begin(); j!=_entities.end(); j++)
			{
				Entity *entity = *j;
				if(entity->HasIntent())
				{
					group.intents.push_back(entity->Intent());
				}
				if(entity->Type() == Entity::Light)
				{
					RenderingLight light = ((LightEntity*)entity)->Light();
					if(camera->InFrustum(light.position, light.range))
					{
						group.lights.push_back(light);
					}
				}
			}
			
			_renderer->PushGroup(group);
		}
		
		_renderer->FinishFrame();
	}
	
	
	
	void World::AddEntity(Entity *entity)
	{
		_entities.push_back(entity);
	}
	
	void World::RemoveEntity(Entity *entity)
	{
		for(auto i=_entities.begin(); i!=_entities.end(); i++)
		{
			if(*i == entity)
			{
				_entities.erase(i);
				return;
			}
		}
	}
	
	void World::AddCamera(Camera *camera)
	{
		_cameras.push_back(camera);
	}
	
	void World::RemoveCamera(Camera *camera)
	{
		for(auto i=_cameras.begin(); i!=_cameras.end(); i++)
		{
			if(*i == camera)
			{
				_cameras.erase(i);
				return;
			}
		}
	}
}
