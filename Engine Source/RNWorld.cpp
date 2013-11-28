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
#include "RNLogging.h"
#include "RNLockGuard.h"
#include "RNThreadPool.h"
#include "RNEntity.h"
#include "RNLight.h"

namespace RN
{
	World::World(SceneManager *sceneManager)
	{
		_kernel = Kernel::GetSharedInstance();
		_kernel->SetWorld(this);
		
		_sceneManager = sceneManager->Retain();
		_cameraClass  = Camera::MetaClass();
		
		_releaseSceneNodesOnDestructor = false;
		_isDroppingSceneNodes = false;
		
		_requiresResort = false;
		_requiresCameraSort = false;
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
	
	
	SceneManager *World::SceneManagerWithName(const std::string& name)
	{
		MetaClassBase *meta = nullptr;
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
	
	void World::UpdatedToFrame(FrameID frame)
	{}
	
	void World::WillRenderSceneNode(SceneNode *node)
	{}
	
	void World::Reset()
	{
		DropSceneNodes();
	}

	
	
	void World::StepWorld(FrameID frame, float delta)
	{
		ApplyNodes();
		Update(delta);
		
		AutoreleasePool pool;
		std::vector<SceneNode *> retry;
		
		ThreadPool::Batch *batch[3];
		size_t run = 0;
		
		do
		{
#if DEBUG
			if(run >= 100)
			{
				Log::Loggable loggable(Log::Level::Warning);
				loggable << "Assuming dead lock after 100 retry runs, check your SceneNode updates!";
				loggable << "Going to break, leaving " << retry.size() << " nodes without update to frame " << frame;
				
				break;
			}
#endif
			
			for(size_t i = 0; i < 3; i ++)
				batch[i] = ThreadPool::GetSharedInstance()->CreateBatch();
			
			for(SceneNode *node : (run == 0) ? _nodes : retry)
			{
				if(node->GetFlags() & SceneNode::FlagStatic)
					continue;
				
				node->Retain();
				pool.AddObject(node);
				
				size_t priority = static_cast<size_t>(node->GetPriority());
				batch[priority]->AddTask([&, node] {
					if(!node->CanUpdate(frame))
					{
						retry.push_back(node);
						return;
					}
					
					node->Update(delta),
					node->UpdatedToFrame(frame);
					
					node->UpdateInternalData();
				});
			}
			
			retry.clear();
			run ++;
			
			for(size_t i = 0; i < 3; i ++)
			{
				batch[i]->Commit();
				batch[i]->Wait();
				batch[i]->Release();
			}
			
		} while(retry.size() > 0);
		
		pool.Drain();
	
		ApplyNodes();
		UpdatedToFrame(frame);
	}
	
	void World::RenderWorld(Renderer *renderer)
	{
		renderer->SetMode(Renderer::Mode::ModeWorld);
		
		for(Camera *camera : _cameras)
		{
			camera->PostUpdate();
			renderer->BeginCamera(camera);
			
			RunWorldAttachement(&WorldAttachment::BeginCamera, camera);
			_sceneManager->RenderScene(camera);
			
			RunWorldAttachement(&WorldAttachment::WillFinishCamera, camera);
			renderer->FinishCamera();
		}
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
		LockGuard<SpinLock> lock(_nodeLock);
		
		if(_isDroppingSceneNodes)
			return;
		
		if(node->_world)
			return;
		
		_addedNodes.push_back(node);
		node->_world = this;
	}
	
	void World::RemoveSceneNode(SceneNode *node)
	{
		LockGuard<SpinLock> lock(_nodeLock);
		
		if(_isDroppingSceneNodes)
			return;
		
		if(node->_world == this)
		{
			_sceneManager->RemoveSceneNode(node);
			
			node->_world = nullptr;
			
			if(node->IsKindOfClass(_cameraClass))
				_cameras.erase(std::remove(_cameras.begin(), _cameras.end(), static_cast<Camera *>(node)), _cameras.end());
			
			auto iterator = std::find(_addedNodes.begin(), _addedNodes.end(), node);
			if(iterator != _addedNodes.end())
			{
				_addedNodes.erase(iterator);
				return;
			}
			
			_nodes.erase(std::find(_nodes.begin(), _nodes.end(), node));
		}
	}
		
	void World::ApplyNodes()
	{
		if(_addedNodes.size() > 0)
		{
			for(SceneNode *node : _addedNodes)
			{
				if(node->IsKindOfClass(_cameraClass))
				{
					_cameras.push_back(static_cast<Camera *>(node));
					_requiresCameraSort = true;
				}
				
				_sceneManager->AddSceneNode(node);
			}
			
			_nodes.insert(_nodes.end(), _addedNodes.begin(), _addedNodes.end());
			_addedNodes.clear();
			
			_requiresResort = true;
		}
		
		if(_requiresResort)
			SortNodes();
		
		if(_requiresCameraSort)
			SortCameras();
	}
	
	
	void World::SceneNodeDidUpdate(SceneNode *node, uint32 changeSet)
	{
		if(_isDroppingSceneNodes)
			return;
		
		_sceneManager->UpdateSceneNode(node, changeSet);
		
		if((changeSet & SceneNode::ChangedPriority) && node->IsKindOfClass(_cameraClass))
			_requiresCameraSort = true;
		
		if(changeSet & SceneNode::ChangedDependencies)
			_requiresResort = true;
	}
	
	void World::SortNodes()
	{
		_requiresResort = false;
		
		std::sort(_nodes.begin(), _nodes.end(), [](const SceneNode *left, const SceneNode *right) {
			return left->Compare(right);
		});
	}
	
	void World::SortCameras()
	{
		_requiresCameraSort = false;
		
		std::stable_sort(_cameras.begin(), _cameras.end(), [](const Camera *left, const Camera *right) {
			return (left->GetPriority() > right->GetPriority());
		});
	}
	
	void World::DropSceneNodes()
	{
		LockGuard<SpinLock> lock(_nodeLock);
		
		_isDroppingSceneNodes = true;
		
		
		
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
