//
//  RNScene.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneWithVisibilityLists.h"
#include "../Debug/RNLogger.h"
#include "../Threads/RNWorkQueue.h"
#include "../Threads/RNWorkGroup.h"
#include "../Objects/RNAutoreleasePool.h"

#define kRNSceneUpdateBatchSize 64

namespace RN
{
	RNDefineMeta(SceneWithVisibilityLists, Scene)
	RNDefineScopedMeta(SceneWithVisibilityLists, Volume, Object)
	RNDefineScopedMeta(SceneWithVisibilityLists, AxisAlignedBoxVolume, SceneWithVisibilityLists::Volume)
	RNDefineMeta(SceneWithVisibilityListsInfo, SceneInfo)
	
	SceneWithVisibilityListsInfo::SceneWithVisibilityListsInfo(Scene *scene) : SceneInfo(scene)
	{
		
	}
	
	bool SceneWithVisibilityLists::Volume::ContainsPosition(const RN::Vector3 &cameraPosition) const
	{
		return true;
	}
	
	bool SceneWithVisibilityLists::AxisAlignedBoxVolume::ContainsPosition(const RN::Vector3 &cameraPosition) const
	{
		return (cameraPosition.z >= boundsMin.z && cameraPosition.y >= boundsMin.y && cameraPosition.x >= boundsMin.x && cameraPosition.x <= boundsMax.x && cameraPosition.y <= boundsMax.y && cameraPosition.z <= boundsMax.z);
	}

	SceneWithVisibilityLists::SceneWithVisibilityLists() : _isAddingVolume(false)
	{
		_volumes = new Array();
		_defaultVolume = new Volume();
	}
	SceneWithVisibilityLists::~SceneWithVisibilityLists()
	{
		SafeRelease(_volumes);
		SafeRelease(_defaultVolume);
	}

	void SceneWithVisibilityLists::Update(float delta)
	{
		WillUpdate(delta);
		
		WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default);
		
		for(size_t i = 0; i < 3; i ++)
		{
			WorkGroup *group = new WorkGroup();
			
			IntrusiveList<SceneNode>::Member *member = _nodes[i].GetHead();
			IntrusiveList<SceneNode>::Member *first = member;
			
			size_t count = 0;
			
			while(member)
			{
				if(count == kRNSceneUpdateBatchSize)
				{
					group->Perform(queue, [&, member, first] {
						
						AutoreleasePool pool;
						auto iterator = first;
						
						while(iterator != member)
						{
							SceneNode *node = iterator->Get();
							
							if(!node->HasFlags(SceneNode::Flags::Static))
							{
								node->Update(delta);
								node->UpdateInternalData();
							}
							
							iterator = iterator->GetNext();
						}
						
					});
					
					first = member;
					count = 0;
				}
				
				member = member->GetNext();
				count ++;
			}
			
			//Update remaining less than kRNSceneUpdateBatchSize number of nodes
			if(first != member)
			{
				group->Perform(queue, [&, member, first] {
					
					AutoreleasePool pool;
					auto iterator = first;
					
					while(iterator != member)
					{
						SceneNode *node = iterator->Get();
						
						if(!node->HasFlags(SceneNode::Flags::Static))
						{
							node->Update(delta);
							node->UpdateInternalData();
						}
						
						iterator = iterator->GetNext();
					}
					
				});
			}
			
			group->Wait();
			group->Release();
		}
		
		Scene::Update(delta);
		
		DidUpdate(delta);
	}
	
	void SceneWithVisibilityLists::RenderVolumeList(int drawPriority, Renderer *renderer, Camera *camera, const Volume *volume)
	{
		for(SceneNode *node : volume->nodes)
		{
			//Skip if this is not the nodes draw priority or other reason (node is a light for example)
			if((drawPriority == 0 && node->IsKindOfClass(Light::GetMetaClass())) || (drawPriority == 1 && (node->GetFlags() & SceneNode::Flags::DrawEarly)) || (drawPriority == 2 && !(node->GetFlags() & SceneNode::Flags::DrawEarly || node->GetFlags() & SceneNode::Flags::DrawLate)) || (drawPriority == 3 && (node->GetFlags() & SceneNode::Flags::DrawLate)))
			{
				if(node->CanRender(renderer, camera))
					node->Render(renderer, camera);
			}
		}
	}
	
	void SceneWithVisibilityLists::Render(Renderer *renderer)
	{
		WillRender(renderer);
		
		for(int cameraPriority = 0; cameraPriority < 3; cameraPriority++)
		{
			IntrusiveList<Camera>::Member *member = _cameras.GetHead();
			while(member)
			{
				Camera *camera = member->Get();
				
				//Early out if camera is not supposed to render or if this isn't it's priority loop
				if(camera->GetFlags() & Camera::Flags::NoRender || (cameraPriority == 0 && !(camera->GetFlags() & Camera::Flags::RenderEarly)) || (cameraPriority == 1 && (camera->GetFlags() & Camera::Flags::RenderEarly || camera->GetFlags() & Camera::Flags::RenderLate)) || (cameraPriority == 2 && !(camera->GetFlags() & Camera::Flags::RenderLate)))
				{
					member = member->GetNext();
					continue;
				}
				
				camera->PostUpdate(renderer);
				Vector3 cameraPosition = camera->GetWorldPosition();
				
				const Object **volumeObjects = _volumes->GetData();
				
				const Object **lowerVolumeObject = volumeObjects;
				const Object **upperVolumeObject = volumeObjects + _volumes->GetCount();
				lowerVolumeObject = std::lower_bound(lowerVolumeObject, upperVolumeObject, cameraPosition, [](const Object *objectA, const Vector3 &worldPosition) -> bool {
					const Volume *volumeA = static_cast<const Volume *>(objectA);
					
					if(volumeA->boundsMax.x < worldPosition.x) return true;
					
					return false;
				});
				
				if(lowerVolumeObject != upperVolumeObject)
				{
					upperVolumeObject = std::upper_bound(lowerVolumeObject, upperVolumeObject, *lowerVolumeObject, [](const Object *objectA, const Object *objectB) -> bool {
						const Volume *volumeA = static_cast<const Volume *>(objectA);
						const Volume *volumeB = static_cast<const Volume *>(objectB);
						
						if(volumeA->boundsMax.x < volumeB->boundsMax.x) return true;
						
						return false;
					});
				}
				
				if(lowerVolumeObject != upperVolumeObject && upperVolumeObject != volumeObjects + _volumes->GetCount())
				{
					lowerVolumeObject = std::lower_bound(lowerVolumeObject, upperVolumeObject, cameraPosition, [](const Object *objectA, const Vector3 &worldPosition) -> bool {
						const Volume *volumeA = static_cast<const Volume *>(objectA);
						
						if(volumeA->boundsMax.y < worldPosition.y) return true;
						
						return false;
					});
					
					if(lowerVolumeObject != upperVolumeObject)
					{
						upperVolumeObject = std::upper_bound(lowerVolumeObject, upperVolumeObject, *lowerVolumeObject, [](const Object *objectA, const Object *objectB) -> bool {
							const Volume *volumeA = static_cast<const Volume *>(objectA);
							const Volume *volumeB = static_cast<const Volume *>(objectB);
							
							if(volumeA->boundsMax.y < volumeB->boundsMax.y) return true;
							
							return false;
						});
					}
					
					if(lowerVolumeObject != upperVolumeObject)
					{
						lowerVolumeObject = std::lower_bound(lowerVolumeObject, upperVolumeObject, cameraPosition, [](const Object *objectA, const Vector3 &worldPosition) -> bool {
							const Volume *volumeA = static_cast<const Volume *>(objectA);
							
							if(volumeA->boundsMax.z < worldPosition.z) return true;
							
							return false;
						});
					}
				}
				
				const Volume *volume = nullptr;
				int offset = 0;
				while(lowerVolumeObject[offset] != volumeObjects[_volumes->GetCount()])
				{
					const Volume *lowerVolume = static_cast<const Volume *>(lowerVolumeObject[offset]);
					if(lowerVolume->ContainsPosition(cameraPosition))
					{
						volume = lowerVolume;
						break;
					}
					
					offset += 1;
				}
				
				renderer->SubmitCamera(camera, [&] {
					for(int drawPriority = 0; drawPriority < 4; drawPriority++)
					{
						RenderVolumeList(drawPriority, renderer, camera, _defaultVolume);
						if(volume)
						{
							RenderVolumeList(drawPriority, renderer, camera, volume);
						}
					}
				});
				
				member = member->GetNext();
			}
		}
		
		DidRender(renderer);
	}
	
	void SceneWithVisibilityLists::AddVolume(Volume *volume)
	{
		_volumes->AddObject(volume);
/*		_volumes->Sort<Volume>([](const Volume *objectA, const Volume *objectB) -> bool {
			if(objectA->boundsMax.x < objectB->boundsMax.x) return true;
			if(objectA->boundsMax.x > objectB->boundsMax.x) return false;
			if(objectA->boundsMax.y < objectB->boundsMax.y) return true;
			if(objectA->boundsMax.y > objectB->boundsMax.y) return false;
			if(objectA->boundsMax.z < objectB->boundsMax.z) return true;
			
			return false;
		});*/
		
		_isAddingVolume = true;
		for(SceneNode *node : volume->nodes)
		{
			if(!node->_sceneInfo)
				AddNode(node);
			
			SceneWithVisibilityListsInfo *sceneInfo = node->GetSceneInfo()->Downcast<SceneWithVisibilityListsInfo>();
			
			if(sceneInfo->volumes.size() == 1 && sceneInfo->volumes[0] == _defaultVolume)
			{
				auto iterator = std::find(_defaultVolume->nodes.begin(), _defaultVolume->nodes.end(), node);
				if(iterator != _defaultVolume->nodes.end())
				{
					_defaultVolume->nodes.erase(iterator);
				}
				sceneInfo->volumes.pop_back();
			}
			
			sceneInfo->volumes.push_back(volume);
		}
		_isAddingVolume = false;
	}
	
	void SceneWithVisibilityLists::AddNode(SceneNode *node)
	{
		RN_ASSERT(node->GetSceneInfo() == nullptr, "AddNode() must be called on a Node not owned by a scene");
		
		if(node->IsKindOfClass(Camera::GetMetaClass()))
		{
			Camera *camera = static_cast<Camera *>(node);
			_cameras.PushBack(camera->_cameraSceneEntry);
		}
		
		_nodes[static_cast<size_t>(node->GetPriority())].PushBack(node->_sceneEntry);
			
		node->Retain();
		SceneWithVisibilityListsInfo *sceneInfo = new SceneWithVisibilityListsInfo(this);
		node->UpdateSceneInfo(sceneInfo->Autorelease());
		
		if(!_isAddingVolume)
		{
			_defaultVolume->nodes.push_back(node);
			sceneInfo->volumes.push_back(_defaultVolume);
		}
	}
	
	void SceneWithVisibilityLists::RemoveNode(SceneNode *node)
	{
		RN_ASSERT(node->GetSceneInfo() && node->GetSceneInfo()->GetScene() == this, "RemoveNode() must be called on a Node owned by the scene");
		
		if(node->IsKindOfClass(Camera::GetMetaClass()))
		{
			Camera *camera = static_cast<Camera *>(node);
			_cameras.Erase(camera->_cameraSceneEntry);
		}
		
		_nodes[static_cast<size_t>(node->GetPriority())].Erase(node->_sceneEntry);
		
		SceneWithVisibilityListsInfo *sceneInfo = node->GetSceneInfo()->Downcast<SceneWithVisibilityListsInfo>();
		for(Volume *volume : sceneInfo->volumes)
		{
			auto iterator = std::find(volume->nodes.begin(), volume->nodes.end(), node);
			if(iterator != volume->nodes.end())
			{
				volume->nodes.erase(iterator);
			}
		}
		
		node->UpdateSceneInfo(nullptr);
		node->Autorelease();
	}
}
