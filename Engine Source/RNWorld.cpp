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
		_cameraClass  = Camera::MetaClass();
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
		MetaClassBase *meta = 0;
		Catalogue::SharedInstance()->EnumerateClasses([&](MetaClassBase *mclass, bool *stop) {
			if(mclass->Name() == name)
			{
				meta = mclass;
				*stop = true;
			}
		});
		
		return static_cast<class SceneManager *>(meta->Construct());
	}
	
	
	
	void World::Update(float delta)
	{}
	
	void World::NodesUpdated()
	{}
	
	void World::WillRenderSceneNode(SceneNode *node)
	{}

	
	void World::StepWorld(FrameID frame, float delta)
	{
		Update(delta);
		ApplyNodes();
		
		for(size_t i=0; i<_attachments.Count(); i++)
		{
			WorldAttachment *attachment = _attachments.ObjectAtIndex<WorldAttachment>(i);
			attachment->StepWorld(delta);
		}
		
		// Add the Transform updates to the thread pool
		ThreadPool::Batch *batch[3];
		SpinLock lock;
		std::vector<SceneNode *> resubmit;
		
		batch[0] = ThreadPool::SharedInstance()->CreateBatch();
		batch[1] = ThreadPool::SharedInstance()->CreateBatch();
		batch[2] = ThreadPool::SharedInstance()->CreateBatch();
		
#define BuildLambda(t) [&, t]() { \
			_deleteLock.Lock(); \
			if(_removedNodes.find(t) != _removedNodes.end()) \
			{ \
				_deleteLock.Unlock(); \
				return; \
			} \
			t->Retain(); \
			_deleteLock.Unlock(); \
			if(!t->CanUpdate(frame)) \
			{ \
				lock.Lock(); \
				resubmit.push_back(t); \
				lock.Unlock(); \
				t->Release(); \
				return; \
			} \
			t->Update(delta); \
			t->WorldTransform(); \
			t->UpdatedToFrame(frame); \
			t->Release(); \
		}
		
		for(auto i=_nodes.begin(); i!=_nodes.end(); i++)
		{
			SceneNode *node = *i;
			batch[static_cast<size_t>(node->UpdatePriority())]->AddTask(BuildLambda(node));
		}
		
		for(size_t i = 0; i < 3; i ++)
		{
			if(batch[i]->TaskCount() > 0)
			{
				bool rerun;
				do
				{				
					rerun = false;
					
					batch[i]->Commit();
					batch[i]->Wait();
					
					batch[i]->Release();
					batch[i] = nullptr;
					
					if(resubmit.size() > 0)
					{
						batch[i] = ThreadPool::SharedInstance()->CreateBatch();
						
						for(SceneNode *node : resubmit)
						{
							batch[i]->AddTask(BuildLambda(node));
						}
						
						resubmit.clear();
						rerun = true;
					}
					
				} while(rerun);
			}
			
			if(batch[i])
				batch[i]->Release();
		}
	
		_removedNodes.clear();
	
		ApplyNodes();
		NodesUpdated();
		
		for(size_t i = 0; i < _attachments.Count(); i ++)
		{
			WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
			attachment->SceneNodesUpdated();
		}
		
		// Iterate over all cameras and render the visible nodes
		std::stable_sort(_cameras.begin(), _cameras.end(), [](const Camera *left, const Camera *right) {
			return (left->Priority() > right->Priority());
		});
		
		_renderer->SetMode(Renderer::Mode::ModeWorld);
		
		for(Camera *camera : _cameras)
		{
			camera->PostUpdate();
			_renderer->BeginCamera(camera);
			
			for(size_t i = 0; i < _attachments.Count(); i ++)
			{
				WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
				attachment->BeginCamera(camera);
			}
			
			_sceneManager->RenderScene(camera);
			
			for(size_t i = 0; i < _attachments.Count(); i ++)
			{
				WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
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
			ForceInsertNode(node);
		}
		
		_addedNodes.clear();
	}
	
	void World::SceneNodeWillRender(SceneNode *node)
	{
		for(size_t i = 0; i < _attachments.Count(); i ++)
		{
			WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
			attachment->WillRenderSceneNode(node);
		}
		
		WillRenderSceneNode(node);
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
		if(!node || node->_world != 0)
			return;
		
		_nodeLock.Lock();
		node->_world = this;
		_addedNodes.push_back(node);
		_nodeLock.Unlock();
	}
	
	void World::RemoveSceneNode(SceneNode *node)
	{
		if(!node || node->_world != this)
			return;
		
		_deleteLock.Lock();
		_removedNodes.insert(node);
		_deleteLock.Unlock();
		
		_nodeLock.Lock();
		
		auto iterator = _nodes.find(node);
		if(iterator != _nodes.end())
		{
			for(size_t i = 0; i < _attachments.Count(); i ++)
			{
				WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
				attachment->WillRemoveSceneNode(node);
			}
			
			if(node->IsKindOfClass(_cameraClass))
			{
				Camera *camera = static_cast<Camera *>(node);
				_cameras.erase(std::remove(_cameras.begin(), _cameras.end(), camera), _cameras.end());
			}
			
			_sceneManager->RemoveSceneNode(node);
			_nodes.erase(iterator);
			
			node->_world = 0;
		}
		
		_addedNodes.erase(std::remove(_addedNodes.begin(), _addedNodes.end(), node), _addedNodes.end());
		_nodeLock.Unlock();
	}
	
	
	void World::SceneNodeUpdated(SceneNode *node)
	{
		if(node->_world != this)
			return;
		
		_nodeLock.Lock();

		auto iterator = std::find(_addedNodes.begin(), _addedNodes.end(), node);
		bool forceInsert = (iterator != _addedNodes.end());
		
		if(forceInsert)
			_addedNodes.erase(iterator);
		
		_nodeLock.Unlock();
		
		if(forceInsert)
			ForceInsertNode(node);
		
		
		for(size_t i = 0; i < _attachments.Count(); i ++)
		{
			WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
			attachment->SceneNodeDidUpdate(node);
		}
		
		_sceneManager->UpdateSceneNode(node);
	}
	
	void World::ForceInsertNode(SceneNode *node)
	{
		if(_nodes.find(node) == _nodes.end())
		{
			_nodes.insert(node);
			_sceneManager->AddSceneNode(node);
			
			for(size_t i = 0; i < _attachments.Count(); i ++)
			{
				WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
				attachment->DidAddSceneNode(node);
			}
			
			if(node->IsKindOfClass(_cameraClass))
			{
				Camera *camera = static_cast<Camera *>(node);
				_cameras.push_back(camera);
			}
		}
	}
}
