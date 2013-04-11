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
	
	void World::NodesUpdated()
	{}
	

	
	void World::StepWorld(FrameID frame, float delta)
	{
		Update(delta);
		ApplyNodes();
		
		for(machine_uint i=0; i<_attachments.Count(); i++)
		{
			WorldAttachment *attachment = _attachments.ObjectAtIndex(i);
			attachment->StepWorld(delta);
		}
		
		ThreadPool *pool = ThreadCoordinator::SharedInstance()->GlobalPool();
		
		// Add the Transform updates to the thread pool
		pool->BeginTaskBatch();
		
		for(auto i=_nodes.begin(); i!=_nodes.end(); i++)
		{
			SceneNode *node = *i;
			node->Retain();
			
			pool->AddTaskWithPredicate([&, node]() {
				node->Update(delta);
				node->WorldTransform(); // Make sure that transforms matrices get updated within the thread pool
				node->UpdatedToFrame(frame);
				node->Release();
			}, [&, node]() { return node->CanUpdate(frame); });
		}
		
		pool->CommitTaskBatch(true);
		
		ApplyNodes();
		NodesUpdated();
		
		for(machine_uint i=0; i<_attachments.Count(); i++)
		{
			WorldAttachment *attachment = _attachments.ObjectAtIndex(i);
			attachment->SceneNodesUpdated();
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
			
			_sceneManager->RenderScene(camera);
			
			for(machine_uint i=0; i<_attachments.Count(); i++)
			{
				WorldAttachment *attachment = _attachments.ObjectAtIndex(i);
				attachment->WillFinishCamera(camera);
			}
			
			_renderer->FinishCamera();
		}
	}
	
	
	void World::ApplyNodes()
	{
		for(auto i=_addedNodes.begin(); i!=_addedNodes.end(); i++)
		{
			SceneNode *node = *i;
			
			if(_nodes.find(node) == _nodes.end())
			{
				node->_world = this;
				
				_nodes.insert(node);
				_sceneManager->AddSceneNode(node);
				
				for(machine_uint i=0; i<_attachments.Count(); i++)
				{
					WorldAttachment *attachment = _attachments.ObjectAtIndex(i);
					attachment->DidAddSceneNode(node);
				}
			}
		}
		
		_addedNodes.clear();
	}

	
	void World::AddAttachment(WorldAttachment *attachment)
	{
		_attachments.AddObject(attachment);
	}
	
	void World::RemoveAttachment(WorldAttachment *attachment)
	{
		_attachments.RemoveObject(attachment);
	}
	
	
	
	
	void World::AddSceneNode(SceneNode *node)
	{
		if(!node)
			return;
		
		_addedNodes.push_back(node);
	}
	
	void World::RemoveSceneNode(SceneNode *node)
	{
		if(!node)
			return;
		
		auto iterator = _nodes.find(node);
		if(iterator != _nodes.end())
		{
			for(machine_uint i=0; i<_attachments.Count(); i++)
			{
				WorldAttachment *attachment = _attachments.ObjectAtIndex(i);
				attachment->WillRemoveSceneNode(node);
			}
			
			_sceneManager->RemoveSceneNode(node);
			_nodes.erase(iterator);
			
			node->_world = 0;
		}
		
		_addedNodes.erase(std::remove(_addedNodes.begin(), _addedNodes.end(), node), _addedNodes.end());
	}
	
	
	void World::SceneNodeUpdated(SceneNode *node)
	{
		auto iterator = std::find(_addedNodes.begin(), _addedNodes.end(), node);
		if(iterator != _addedNodes.end())
		{
			_addedNodes.erase(iterator);
			
			_nodes.insert(node);
			_sceneManager->AddSceneNode(node);
		}
		
		_sceneManager->UpdateSceneNode(node);
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
