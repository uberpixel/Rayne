//
//  RNWorld.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorld.h"
#include "RNKernel.h"
#include "RNAutoreleasePool.h"
#include "RNWorldCoordinator.h"
#include "RNLogging.h"
#include "RNLockGuard.h"
#include "RNThreadPool.h"
#include "RNEntity.h"
#include "RNLight.h"
#include "RNLogging.h"

namespace RN
{
	RNDefineMeta(World)
	
	World::World(SceneManager *sceneManager) :
		_releaseSceneNodesOnDestructor(true),
		_isDroppingSceneNodes(false),
		_requiresCameraSort(false),
		_requiresResort(false),
		_mode(Mode::Play)
	{
		_kernel = Kernel::GetSharedInstance();
		
		_sceneManager = sceneManager->Retain();
		_cameraClass  = Camera::MetaClass();
	}
	
	World::World(const std::string& sceneManager) :
		World(World::SceneManagerWithName(sceneManager))
	{}
	
	World::~World()
	{
		if(_releaseSceneNodesOnDestructor)
			DropSceneNodes();
		
		_sceneManager->Release();
	}
	
	SceneManager *World::SceneManagerWithName(const std::string& name)
	{
		MetaClassBase *meta = nullptr;
		Catalogue::GetSharedInstance()->EnumerateClasses([&](MetaClassBase *mclass, bool &stop) {
			if(mclass->Name() == name)
			{
				meta = mclass;
				stop = true;
			}
		});
		
		return static_cast<class SceneManager *>(meta->Construct());
	}
	
	World *World::GetActiveWorld()
	{
		return WorldCoordinator::GetSharedInstance()->GetWorld();
	}
	
	
	void World::SetReleaseSceneNodesOnDestruction(bool releaseSceneNodes)
	{
		_releaseSceneNodesOnDestructor = releaseSceneNodes;
	}
	
	void World::SetMode(Mode mode)
	{
		_kernel->ScheduleFunction([this, mode] {
			_mode = mode;
		});
	}
	
	// ---------------------
	// MARK: -
	// MARK: Callbacks
	// ---------------------
	
	void World::Update(float delta)
	{}
	
	void World::UpdateEditMode(float delta)
	{}
	
	void World::DidUpdateToFrame(FrameID frame)
	{}
	
	// ---------------------
	// MARK: -
	// MARK: Updating
	// ---------------------
	
	void World::StepWorldEditMode(FrameID frame, float delta)
	{
		ApplyNodes();
		UpdateEditMode(delta);
		
		AutoreleasePool pool;
		std::vector<SceneNode *> retry;
		
		ThreadPool::Batch *batch[3];
		size_t run = 0;
		
		SpinLock retryLock;
		
		do
		{
#if RN_BUILD_DEBUG
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
				node->Retain();
				pool.AddObject(node);
				
				size_t priority = static_cast<size_t>(node->GetPriority());
				batch[priority]->AddTask([&, node] {
					if(!node->CanUpdate(frame))
					{
						retryLock.Lock();
						retry.push_back(node);
						retryLock.Unlock();
						
						return;
					}
					
					node->UpdateEditMode(delta),
					node->UpdatedToFrame(frame);
					
					node->GetWorldPosition();
				});
			}
			
			retry.clear();
			run ++;
			
			for(size_t i = 0; i < 3; i ++)
			{
				if(batch[i]->GetTaskCount() > 0)
				{
					batch[i]->Commit();
					batch[i]->Wait();
				}
				
				batch[i]->Release();
			}
			
		} while(retry.size() > 0);
		
		pool.Drain();
		
		ApplyNodes();
		DidUpdateToFrame(frame);
		RunWorldAttachement(&WorldAttachment::StepWorldEditMode, delta);
	}
	
	void World::StepWorld(FrameID frame, float delta)
	{
		_kernel->PushStatistics("wld.step");
		
		if(_mode == Mode::Edit)
		{
			StepWorldEditMode(frame, delta);
			_kernel->PopStatistics();
			
			return;
		}
		
		ApplyNodes();
		Update(delta);
		
		AutoreleasePool pool;
		std::vector<SceneNode *> retry;
		
		ThreadPool::Batch *batch[3];
		size_t run = 0;
		
		SpinLock retryLock;
		
		do
		{
#if RN_BUILD_DEBUG
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
				node->Retain();
				pool.AddObject(node);
				
				size_t priority = static_cast<size_t>(node->GetPriority());
				batch[priority]->AddTask([&, node] {
					if(!node->CanUpdate(frame))
					{
						retryLock.Lock();
						retry.push_back(node);
						retryLock.Unlock();
						
						return;
					}
					
					node->Update(delta),
					node->UpdatedToFrame(frame);
					
					node->GetWorldPosition();
				});
			}
			
			retry.clear();
			run ++;
			
			for(size_t i = 0; i < 3; i ++)
			{
				if(batch[i]->GetTaskCount() > 0)
				{
					batch[i]->Commit();
					batch[i]->Wait();
				}
				
				batch[i]->Release();
			}
			
		} while(retry.size() > 0);
		
		pool.Drain();
	
		ApplyNodes();
		DidUpdateToFrame(frame);
		RunWorldAttachement(&WorldAttachment::StepWorld, delta);
		
		_kernel->PopStatistics();
	}
	
	void World::RenderWorld(Renderer *renderer)
	{
		_kernel->PushStatistics("wld.render");
		
		renderer->SetMode(Renderer::Mode::ModeWorld);
		
		for(Camera *camera : _cameras)
		{
			camera->PostUpdate();
			
			if(camera->GetFlags() & Camera::Flags::Hidden)
				continue;
			
			renderer->BeginCamera(camera);
			
			RunWorldAttachement(&WorldAttachment::DidBeginCamera, camera);
			_sceneManager->RenderScene(camera);
			
			RunWorldAttachement(&WorldAttachment::WillFinishCamera, camera);
			renderer->FinishCamera();
		}
		
		_kernel->PopStatistics();
	}
	
	void World::SceneNodeWillRender(SceneNode *node)
	{
		RunWorldAttachement(&WorldAttachment::WillRenderSceneNode, node);
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Scene Nodes
	// ---------------------
	
	void World::AddSceneNode(SceneNode *node)
	{
		LockGuard<decltype(_nodeLock)> lock(_nodeLock);
		
		if(_isDroppingSceneNodes)
			return;
		
		if(node->_world)
			return;
		
		_addedNodes.push_back(node);
		
		node->_world = this;
		node->_worldInserted = false;
		node->Retain();
	}
	
	void World::RemoveSceneNode(SceneNode *node)
	{
		if(__RemoveSceneNode(node))
		{
			node->Release();
		}
	}
	
	bool World::__RemoveSceneNode(SceneNode *node)
	{
		LockGuard<decltype(_nodeLock)> lock(_nodeLock);
		
		if(_isDroppingSceneNodes)
			return false;
		
		if(node->_world == this)
		{
			DropSceneNode(node);
			
			if(node->IsKindOfClass(_cameraClass))
				_cameras.erase(std::remove(_cameras.begin(), _cameras.end(), static_cast<Camera *>(node)), _cameras.end());
			
			auto iterator = std::find(_addedNodes.begin(), _addedNodes.end(), node);
			if(iterator != _addedNodes.end())
			{
				_addedNodes.erase(iterator);
				return true;
			}
			
			if(!node->_worldStatic)
			{
				_nodes.erase(std::find(_nodes.begin(), _nodes.end(), node));
			}
			else
			{
				_staticNodes.erase(std::find(_staticNodes.begin(), _staticNodes.end(), node));
			}
			
			return true;
		}
		
		return false;
	}
		
	void World::ApplyNodes()
	{
		if(_addedNodes.size() > 0)
		{
			for(SceneNode *node : _addedNodes)
			{
				node->_worldInserted = true;
				
				if(node->IsKindOfClass(_cameraClass))
				{
					_cameras.push_back(static_cast<Camera *>(node));
					_requiresCameraSort = true;
				}
				
				_sceneManager->AddSceneNode(node);
				
				if(node->GetFlags() & SceneNode::Flags::Static)
				{
					_staticNodes.push_back(node);
					node->_worldStatic = true;
				}
				else
				{
					_nodes.push_back(node);
					node->_worldStatic = false;
				}
				
				Tag tag = node->GetTag();
				if(tag != 0)
				{
					std::unordered_set<SceneNode *> &table = _tagTable[tag];
					table.insert(node);
				}
				
				RunWorldAttachement(&WorldAttachment::DidAddSceneNode, node);
				node->DidUpdate(SceneNode::ChangeSet::World);
			}
			
			_addedNodes.clear();
			_requiresResort = true;
		}
		
		if(_requiresResort)
			SortNodes();
		
		if(_requiresCameraSort)
			SortCameras();
	}
	
	
	void World::SceneNodeWillUpdate(SceneNode *node, SceneNode::ChangeSet changeSet)
	{
		if(changeSet & SceneNode::ChangeSet::Tag)
		{
			Tag tag = node->GetTag();
			if(tag != 0)
			{
				LockGuard<decltype(_nodeLock)> lock(_nodeLock);
				
				std::unordered_set<SceneNode *> &table = _tagTable[tag];
				table.erase(node);
				
				if(table.empty())
					_tagTable.erase(tag);
			}
		}
	}
	
	void World::SceneNodeDidUpdate(SceneNode *node, SceneNode::ChangeSet changeSet)
	{
		if(_isDroppingSceneNodes)
			return;
		
		if(!node->_worldInserted)
			return;
		
		RunWorldAttachement(&WorldAttachment::SceneNodeDidUpdate, node, changeSet);
		_sceneManager->UpdateSceneNode(node, changeSet);
		
		if((changeSet & SceneNode::ChangeSet::Priority) && node->IsKindOfClass(_cameraClass))
			_requiresCameraSort = true;
		
		if((changeSet & SceneNode::ChangeSet::Dependencies) && !node->_worldStatic)
			_requiresResort = true;
		
		if(changeSet & SceneNode::ChangeSet::Flags)
		{
			if(node->GetFlags() & SceneNode::Flags::Static)
			{
				if(!node->_worldStatic)
				{
					LockGuard<decltype(_nodeLock)> lock(_nodeLock);
					
					_nodes.erase(std::find(_nodes.begin(), _nodes.end(), node));
					_staticNodes.push_back(node);
					
					node->_worldStatic = true;
				}
			}
			else
			{
				if(node->_worldStatic)
				{
					LockGuard<decltype(_nodeLock)> lock(_nodeLock);
					
					_requiresResort = true;
					_staticNodes.erase(std::find(_staticNodes.begin(), _staticNodes.end(), node));
					_nodes.push_back(node);
					
					node->_worldStatic = false;
				}
			}
		}
		
		if(changeSet & SceneNode::ChangeSet::Tag)
		{
			Tag tag = node->GetTag();
			if(tag != 0)
			{
				LockGuard<decltype(_nodeLock)> lock(_nodeLock);
				
				std::unordered_set<SceneNode *> &table = _tagTable[tag];
				table.insert(node);
			}
		}
	}
	
	void World::DropSceneNode(SceneNode *node)
	{
		RunWorldAttachement(&WorldAttachment::WillRemoveSceneNode, node);
		_sceneManager->RemoveSceneNode(node);
		
		node->_world = nullptr;
		node->DidUpdate(SceneNode::ChangeSet::World);
	}
	
	void World::DropSceneNodes()
	{
		LockGuard<decltype(_nodeLock)> lock(_nodeLock);
		
		_isDroppingSceneNodes = true;
		
		for(SceneNode *node : _nodes)
		{
			DropSceneNode(node);
			node->Release();
		}
		
		for(SceneNode *node : _addedNodes)
		{
			DropSceneNode(node);
			node->Release();
		}
		
		for(SceneNode *node : _staticNodes)
		{
			DropSceneNode(node);
			node->Release();
		}
		
		
		_nodes.clear();
		_addedNodes.clear();
		_staticNodes.clear();
		_cameras.clear();
		
		_isDroppingSceneNodes = false;
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
	
	SceneNode *World::__GetSceneNodeWithTag(Tag tag, MetaClassBase *meta)
	{
		LockGuard<decltype(_nodeLock)> lock(_nodeLock);
		
		if(_isDroppingSceneNodes)
			return nullptr;
		
		ApplyNodes();
		
		if(tag == 0)
		{
			for(SceneNode *node : _nodes)
			{
				if(node->_tag == tag && node->IsKindOfClass(meta))
					return node->Retain()->Autorelease();
			}
		}
		else
		{
			auto iterator = _tagTable.find(tag);
			if(iterator == _tagTable.end())
				return nullptr;
			
			std::unordered_set<SceneNode *> &table = iterator->second;
			for(SceneNode *node : table)
			{
				if(node->IsKindOfClass(meta))
					return node->Retain()->Autorelease();
			}
		}
		
		return nullptr;
	}
	
	Array *World::__GetSceneNodesWithTag(Tag tag, MetaClassBase *meta)
	{
		LockGuard<decltype(_nodeLock)> lock(_nodeLock);
		
		if(_isDroppingSceneNodes)
			return nullptr;
		
		ApplyNodes();
		Array *array = new Array();
		
		if(tag == 0)
		{
			for(SceneNode *node : _nodes)
			{
				if(node->_tag == tag && node->IsKindOfClass(meta))
					array->AddObject(node);
			}
		}
		else
		{
			auto iterator = _tagTable.find(tag);
			if(iterator == _tagTable.end())
				return nullptr;
			
			std::unordered_set<SceneNode *> &table = iterator->second;
			for(SceneNode *node : table)
			{
				if(node->IsKindOfClass(meta))
					array->AddObject(node);
			}
		}
		
		return array->Autorelease();
	}
	
	Array *World::GetSceneNodes()
	{
		LockGuard<decltype(_nodeLock)> lock(_nodeLock);
		
		if(_isDroppingSceneNodes)
			return Array::WithObjects(nullptr);
		
		Array *array = new Array(_nodes.size() + _addedNodes.size() + _staticNodes.size() + _cameras.size());
		
		for(SceneNode *node : _nodes)
			array->AddObject(node);
		
		for(SceneNode *node : _addedNodes)
			array->AddObject(node);
		
		for(SceneNode *node : _staticNodes)
			array->AddObject(node);
		
		return array->Autorelease();
	}
	
	// ---------------------
	// MARK: -
	// MARK: Attachments
	// ---------------------


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
	
	// ---------------------
	// MARK: -
	// MARK: Loading / Saving
	// ---------------------
	
	void World::LoadOnThread(Thread *thread, Deserializer *deserializer)
	{
		DropSceneNodes();
		
		if(deserializer)
		{
			int32 version = deserializer->DecodeInt32();
			RN_ASSERT(version == 0, "World was encoded with an invalid version (%i)", version);
			
			_sceneManager->Release();
			_sceneManager = SceneManagerWithName(deserializer->DecodeString())->Retain();
			
			_releaseSceneNodesOnDestructor = deserializer->DecodeBool();
			
			size_t nodes = static_cast<size_t>(deserializer->DecodeInt64());
			
			for(size_t i = 0; i < nodes; i ++)
			{
				SceneNode *node = static_cast<SceneNode *>(deserializer->DecodeObject());
				
				if(node)
					AddSceneNode(node);
			}
		}
	}
	
	void World::FinishLoading(Deserializer *deserializer)
	{
		ApplyNodes();
	}
	
	bool World::SupportsBackgroundLoading() const
	{
		return true;
	}
	
	void World::SaveOnThread(Thread *thread, Serializer *serializer)
	{
		LockGuard<decltype(_nodeLock)> lock(_nodeLock);
		ApplyNodes();
		
		serializer->EncodeInt32(0);
		serializer->EncodeString(_sceneManager->Class()->Name()),
		serializer->EncodeBool(_releaseSceneNodesOnDestructor);
		
		serializer->EncodeInt64(static_cast<int64>(_nodes.size()));
		
		std::vector<SceneNode *> saveables;
		
		for(SceneNode *node : _nodes)
		{
			bool noSave = (node->_flags & SceneNode::Flags::NoSave);
			
			if(noSave)
				serializer->EncodeConditionalObject(node);
			else
				serializer->EncodeObject(node);
		}
	}
	
	void World::FinishSaving(Serializer *serializer)
	{}
	
	bool World::SupportsBackgroundSaving() const
	{
		return false;
	}
}
