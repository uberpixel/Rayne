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
#include "RNLockGuard.h"
#include "RNThreadPool.h"
#include "RNEntity.h"
#include "RNLight.h"

namespace RN
{
	World::World(class SceneManager *sceneManager)
	{
		_kernel = Kernel::GetSharedInstance();
		_kernel->SetWorld(this);
		
		_renderer = Renderer::GetSharedInstance();
		_sceneManager = sceneManager->Retain();
		_cameraClass  = Camera::MetaClass();
		
		_releaseSceneNodesOnDestructor = false;
		_isDroppingSceneNodes = false;
	}
	
	World::World(const std::string& sceneManager) :
		World(World::SceneManagerWithName(sceneManager))
	{}
	
	World::~World()
	{
		if(_releaseSceneNodesOnDestructor)
			DropSceneNodes();
		
		_kernel->SetWorld(0);
		_sceneManager->Release();
	}
	
	
	class SceneManager *World::SceneManagerWithName(const std::string& name)
	{
		MetaClassBase *meta = 0;
		Catalogue::GetSharedInstance()->EnumerateClasses([&](MetaClassBase *mclass, bool *stop) {
			if(mclass->Name() == name)
			{
				meta = mclass;
				*stop = true;
			}
		});
		
		return static_cast<class SceneManager *>(meta->Construct());
	}
	
	
	void World::SetReleaseSceneNodesOnDestruction(bool releaseSceneNodes)
	{
		_releaseSceneNodesOnDestructor = releaseSceneNodes;
	}
	
	
	void World::Update(float delta)
	{}
	
	void World::NodesUpdated()
	{}
	
	void World::WillRenderSceneNode(SceneNode *node)
	{}
	
	
	void World::Reset()
	{
		DropSceneNodes();
	}

	
	void World::StepWorld(FrameID frame, float delta)
	{
		Update(delta);
		ApplyNodes();
		
		for(size_t i=0; i<_attachments.GetCount(); i++)
		{
			WorldAttachment *attachment = _attachments.GetObjectAtIndex<WorldAttachment>(i);
			attachment->StepWorld(delta);
		}
		
		// Add the Transform updates to the thread pool
		/*ThreadPool::Batch *batch[3];
		SpinLock lock;
		std::vector<SceneNode *> resubmit;
		
		batch[0] = ThreadPool::GetSharedInstance()->CreateBatch();
		batch[1] = ThreadPool::GetSharedInstance()->CreateBatch();
		batch[2] = ThreadPool::GetSharedInstance()->CreateBatch();
		
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
			t->Lock(); \
			t->Update(delta); \
			t->GetWorldTransform(); \
			t->UpdatedToFrame(frame); \
			t->Unlock(); \
			t->Release(); \
		}
		
		for(auto i=_nodes.begin(); i!=_nodes.end(); i++)
		{
			SceneNode *node = *i;
			batch[static_cast<size_t>(node->GetUpdatePriority())]->AddTask(BuildLambda(node));
		}
		
		for(size_t i = 0; i < 3; i ++)
		{
			if(batch[i]->GetTaskCount() > 0)
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
						batch[i] = ThreadPool::GetSharedInstance()->CreateBatch();
						
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
		}*/
		
		std::vector<SceneNode *> nodes(_nodes.begin(), _nodes.end());
		
		for(size_t j = 0; j < 3; j ++)
		{
			std::vector<SceneNode *> retry;
			
			for(auto i=nodes.begin(); i!=nodes.end(); i++)
			{
				SceneNode *node = *i;
				
				if(_removedNodes.find(node) != _removedNodes.end())
					continue;
				
				if(static_cast<size_t>(node->GetUpdatePriority()) != j)
					continue;
				
				if(!node->CanUpdate(frame))
				{
					retry.push_back(node);
					continue;
				}
				
				node->Update(delta);
				node->UpdatedToFrame(frame);
			}
			
			while(retry.size() > 0)
			{
				std::random_shuffle(retry.begin(), retry.end());
				
				std::vector<SceneNode *> copy(retry);
				retry.clear();
				
				for(auto i=copy.begin(); i!=copy.end(); i++)
				{
					SceneNode *node = *i;
					
					if(_removedNodes.find(node) != _removedNodes.end())
						continue;
					
					if(!node->CanUpdate(frame))
					{
						retry.push_back(node);
						continue;
					}
					
					node->Update(delta);
					node->UpdatedToFrame(frame);
				}
			}
		}
	
		ApplyNodes();
		NodesUpdated();
		
		for(size_t i = 0; i < _attachments.GetCount(); i ++)
		{
			WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
			attachment->SceneNodesUpdated();
		}
		
		// Iterate over all cameras and render the visible nodes
		std::stable_sort(_cameras.begin(), _cameras.end(), [](const Camera *left, const Camera *right) {
			return (left->GetPriority() > right->GetPriority());
		});
		
		_renderer->SetMode(Renderer::Mode::ModeWorld);
		
		for(Camera *camera : _cameras)
		{
			camera->PostUpdate();
			_renderer->BeginCamera(camera);
			
			for(size_t i = 0; i < _attachments.GetCount(); i ++)
			{
				WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
				attachment->BeginCamera(camera);
			}
			
			_sceneManager->RenderScene(camera);
			
			for(size_t i = 0; i < _attachments.GetCount(); i ++)
			{
				WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
				attachment->WillFinishCamera(camera);
			}
			
			_renderer->FinishCamera();
		}
	}
	
	
	void World::ApplyNodes()
	{
		// Added nodes
		for(auto i=_addedNodes.begin(); i!=_addedNodes.end(); i++)
		{
			SceneNode *node = *i;
			ForceInsertNode(node);
		}
		
		_addedNodes.clear();
		
		// Updated nodes
		LockGuard<Array> lock(_attachments);
		
		for(auto i = _updatedNodes.begin(); i != _updatedNodes.end(); i ++)
		{
			SceneNode *node = *i;
			
			if(_removedNodes.find(node) != _removedNodes.end())
				continue;
			
			for(size_t i = 0; i < _attachments.GetCount(); i ++)
			{
				WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
				attachment->SceneNodeDidUpdate(node);
			}
			
			_sceneManager->UpdateSceneNode(node);
		}
		
		_updatedNodes.clear();
		_removedNodes.clear();
	}
	
	void World::SceneNodeWillRender(SceneNode *node)
	{
		for(size_t i = 0; i < _attachments.GetCount(); i ++)
		{
			WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
			attachment->WillRenderSceneNode(node);
		}
		
		WillRenderSceneNode(node);
	}
	
	void World::AddSceneNode(SceneNode *node)
	{
		if(!node || node->_world != 0)
			return;
		
		if(_isDroppingSceneNodes)
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
		
		if(_isDroppingSceneNodes)
			return;
		
		_deleteLock.Lock();
		_removedNodes.insert(node);
		_deleteLock.Unlock();
		
		_nodeLock.Lock();
		
		auto iterator = _nodes.find(node);
		if(iterator != _nodes.end())
		{
			LockGuard<Array> lock(_attachments);
			
			for(size_t i = 0; i < _attachments.GetCount(); i ++)
			{
				WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
				attachment->WillRemoveSceneNode(node);
			}
			
			lock.Unlock();
			
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
		if(_isDroppingSceneNodes)
			return;
		
		_nodeLock.Lock();

		auto iterator = std::find(_addedNodes.begin(), _addedNodes.end(), node);
		bool forceInsert = (iterator != _addedNodes.end());
		
		if(forceInsert)
			_addedNodes.erase(iterator);
		
		_updatedNodes.insert(node);
		_nodeLock.Unlock();
		
		if(forceInsert)
		{
			ForceInsertNode(node);
			return;
		}
	}
	
	void World::ForceInsertNode(SceneNode *node)
	{
		if(_nodes.find(node) == _nodes.end())
		{
			_nodes.insert(node);
			_sceneManager->AddSceneNode(node);
			
			//LockGuard<Array> lock(_attachments);
			
			for(size_t i = 0; i < _attachments.GetCount(); i ++)
			{
				WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
				attachment->DidAddSceneNode(node);
			}
			
			//lock.Unlock();
			
			if(node->IsKindOfClass(_cameraClass))
			{
				Camera *camera = static_cast<Camera *>(node);
				_cameras.push_back(camera);
			}
		}
	}

	void World::DropSceneNodes()
	{
		LockGuard<SpinLock> nodeLock(_nodeLock);
		LockGuard<SpinLock> deleteLock(_deleteLock);
		LockGuard<Array> attachmentLock(_attachments);
		
		for(SceneNode *node : _removedNodes)
			_nodes.erase(node);
		
		_removedNodes.clear();
		_isDroppingSceneNodes = true;
		
		std::vector<SceneNode *> dropNodes;
		
		for(SceneNode *node : _nodes)
		{
			for(size_t i = 0; i < _attachments.GetCount(); i ++)
			{
				WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
				attachment->WillRemoveSceneNode(node);
			}
			
			_sceneManager->RemoveSceneNode(node);
			
			if(!node->GetParent())
				dropNodes.push_back(node);
		}
		
		for(SceneNode *node : dropNodes)
		{
			node->_world = nullptr;
			node->Release();
		}
		
		_nodes.clear();
		_updatedNodes.clear();
		_addedNodes.clear();
		_cameras.clear();
		
		_isDroppingSceneNodes = false;
	}




	void World::AddAttachment(WorldAttachment *attachment)
	{
		LockGuard<Array> lock(_attachments);
		
		_attachments.AddObject(attachment);
	}

	void World::RemoveAttachment(WorldAttachment *attachment)
	{
		LockGuard<Array> lock(_attachments);
		
		_attachments.RemoveObject(attachment);
	}
}
