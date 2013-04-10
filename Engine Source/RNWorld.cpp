//
//  RNWorld.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorld.h"
#include "RNKernel.h"
#include "RNAutoreleasePool.h"
#include "RNThreadPool.h"
#include "RNEntity.h"
#include "RNLight.h"

namespace RN
{
	World::World(class SceneManager *sceneManager)
	{
		_kernel = Kernel::SharedInstance();
		_kernel->SetWorld(this);
		
		_renderer = Renderer::SharedInstance();
		_sceneManager = sceneManager->Retain();
	}
	
	World::World(const std::string& sceneManager) :
		World(World::SceneManagerWithName(sceneManager))
	{
	}
	
	World::~World()
	{
		_kernel->SetWorld(0);
		_sceneManager->Release();
	}
	
	
	class SceneManager *World::SceneManagerWithName(const std::string& name)
	{
		class MetaClass *meta = Catalogue::SharedInstance()->ClassWithName(name);
		return static_cast<class SceneManager *>(meta->Construct()->Autorelease());
	}
	
	
	
	void World::Update(float delta)
	{}
	
	void World::TransformsUpdated()
	{}
	

	
	void World::StepWorld(FrameID frame, float delta)
	{
		Update(delta);
		
		for(machine_uint i=0; i<_attachments.Count(); i++)
		{
			WorldAttachment *attachment = _attachments.ObjectAtIndex(i);
			attachment->StepWorld(delta);
		}
		
		ThreadPool *pool = ThreadCoordinator::SharedInstance()->GlobalPool();
		
		// Add the Transform updates to the thread pool
		pool->BeginTaskBatch();
		
		for(auto i=_transforms.begin(); i!=_transforms.end(); i++)
		{
			Transform *transform = *i;
			transform->Retain();
			
			pool->AddTaskWithPredicate([&, transform]() {
				transform->Update(delta);
				transform->WorldTransform(); // Make sure that transforms matrices get updated within the thread pool
				transform->UpdatedToFrame(frame);
				transform->Release();
			}, [&, transform]() { return transform->CanUpdate(frame); });
		}
		
		pool->CommitTaskBatch(true);		
		TransformsUpdated();
		
		for(machine_uint i=0; i<_attachments.Count(); i++)
		{
			WorldAttachment *attachment = _attachments.ObjectAtIndex(i);
			attachment->TransformsUpdated();
		}
		
		// Iterate over all cameras and render the visible nodes
		for(Camera *camera : _cameras)
		{
			camera->PostUpdate();
			_renderer->BeginCamera(camera);
			
			for(machine_uint i=0; i<_attachments.Count(); i++)
			{
				WorldAttachment *attachment = _attachments.ObjectAtIndex(i);
				attachment->BeginCamera(camera);
			}
			
			_sceneManager->RenderTransforms(camera);
			
			for(machine_uint i=0; i<_attachments.Count(); i++)
			{
				WorldAttachment *attachment = _attachments.ObjectAtIndex(i);
				attachment->WillFinishCamera(camera);
			}
			
			_renderer->FinishCamera();
		}
	}
	

	
	void World::AddAttachment(WorldAttachment *attachment)
	{
		_attachments.AddObject(attachment);
	}
	
	void World::RemoveAttachment(WorldAttachment *attachment)
	{
		_attachments.RemoveObject(attachment);
	}
	
	
	
	
	void World::AddTransform(Transform *transform)
	{
		if(!transform)
			return;
		
		if(_transforms.find(transform) == _transforms.end())
		{
			_transforms.insert(transform);
			_sceneManager->AddTransform(transform);
			
			for(machine_uint i=0; i<_attachments.Count(); i++)
			{
				WorldAttachment *attachment = _attachments.ObjectAtIndex(i);
				attachment->DidAddTransform(transform);
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
			for(machine_uint i=0; i<_attachments.Count(); i++)
			{
				WorldAttachment *attachment = _attachments.ObjectAtIndex(i);
				attachment->WillRemoveTransform(transform);
			}
			
			_sceneManager->RemoveTransform(transform);
			_transforms.erase(iterator);
		}

	}
	
	
	void World::TransformUpdated(Transform *transform)
	{
		_sceneManager->UpdateTransform(transform);
	}
	
	
	void World::AddCamera(Camera *camera)
	{
		_cameras.push_back(camera);
	}
	
	void World::RemoveCamera(Camera *camera)
	{
		_cameras.erase(std::remove(_cameras.begin(), _cameras.end(), camera), _cameras.end());
	}
}
