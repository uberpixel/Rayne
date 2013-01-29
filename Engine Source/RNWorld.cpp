//
//  RNWorld.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorld.h"
#include "RNKernel.h"

namespace RN
{
	World::World()
	{
		_kernel = Kernel::SharedInstance();
		_kernel->SetWorld(this);
		
		_cameras = new ObjectArray();
		
		_renderer = _kernel->Renderer();
		_physics  = new PhysicsPipeline();
		
		Rect frame = _kernel->Window()->Frame();
		Camera *camera = new Camera(Vector2(frame.width, frame.height));
		
		_cameras->AddObject(camera);
		camera->Release();
		
		_physicsTask = kPipelineSegmentNullTask;
		_renderingTask = kPipelineSegmentNullTask;
	}
	
	World::~World()
	{
		_cameras->Release();
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
		
		for(machine_uint i=0; i<_cameras->Count(); i++)
		{
			Camera *camera = (Camera *)_cameras->ObjectAtIndex(i);
			
			RenderingGroup group;
			group.camera = camera;
			
			for(auto i=_entities.begin(); i!=_entities.end(); i++)
			{
				Entity *entity = *i;
				group.intents.push_back(entity->Intent());
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
}
