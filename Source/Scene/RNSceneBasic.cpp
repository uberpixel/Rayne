//
//  RNScene.cpp
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneBasic.h"
#include "../Debug/RNLogger.h"
#include "../Threads/RNWorkQueue.h"
#include "../Threads/RNWorkGroup.h"
#include "../Objects/RNAutoreleasePool.h"

#define kRNSceneUpdateBatchSize 64
#define kRNSceneRenderBatchSize 32

namespace RN
{
	RNDefineMeta(SceneBasic, Scene)

	SceneBasic::SceneBasic() : _nodesToRemove(new Array())
	{
		
	}
	
	SceneBasic::~SceneBasic()
	{
		_nodesToRemove->Release();
	}

	void SceneBasic::Update(float delta)
	{
		WillUpdate(delta);

		WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default);

		for(size_t i = 0; i < 3; i ++)
		{
			WorkGroup *group = new WorkGroup();

			IntrusiveList<SceneNode>::Member *member = _updateNodes[i].GetHead();
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
							UpdateNode(node, delta);
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
						UpdateNode(node, delta);
						iterator = iterator->GetNext();
					}

				});
			}

			group->Wait();
			group->Release();
		}
		
		FlushDeletionQueue();

		Scene::Update(delta);

		DidUpdate(delta);
	}

	void SceneBasic::FlushDeletionQueue()
	{
		_nodesToRemove->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {

			if(node->IsKindOfClass(Camera::GetMetaClass()))
			{
				Camera *camera = static_cast<Camera *>(node);
				_cameras.Erase(camera->_cameraSceneEntry);
			}
			else if(node->IsKindOfClass(Light::GetMetaClass()))
			{
				Light *light = static_cast<Light *>(node);
				_lights.Erase(light->_lightSceneEntry);
			}
			else
			{
				RemoveRenderNode(node);
			}

			_updateNodes[static_cast<size_t>(node->GetUpdatePriority())].Erase(node->_sceneUpdateEntry);

			node->UpdateSceneInfo(nullptr);
			node->Autorelease();
		});

		_nodesToRemove->RemoveAllObjects();
	}

	void SceneBasic::Render(Renderer *renderer)
	{
		WillRender(renderer);
		
		//Run camera PostUpdate once for each camera
		IntrusiveList<Camera>::Member *cameraMember = _cameras.GetHead();
		while(cameraMember)
		{
			Camera *camera = cameraMember->Get();
			camera->PostUpdate();
			cameraMember = cameraMember->GetNext();
		}

		for(int cameraPriority = 0; cameraPriority < 3; cameraPriority++)
		{
			cameraMember = _cameras.GetHead();
			while(cameraMember)
			{
				Camera *camera = cameraMember->Get();

				//Early out if camera is not supposed to render or if this isn't it's priority loop
				if(camera->GetFlags() & Camera::Flags::NoRender || (cameraPriority == 0 && !(camera->GetFlags() & Camera::Flags::RenderEarly)) || (cameraPriority == 1 && (camera->GetFlags() & Camera::Flags::RenderEarly || camera->GetFlags() & Camera::Flags::RenderLate)) || (cameraPriority == 2 && !(camera->GetFlags() & Camera::Flags::RenderLate)))
				{
					cameraMember = cameraMember->GetNext();
					continue;
				}
				
				//Multiview cameras need to be skipped, they are rendered through their parent camera
				if(camera->GetIsMultiviewCamera())
				{
					cameraMember = cameraMember->GetNext();
					continue;
				}

				renderer->SubmitCamera(camera, [&] {
					
					//TODO: Add back some multithreading while not breaking the priorities.
					
					//Submit lights first
					IntrusiveList<Light>::Member *lightMember = _lights.GetHead();
					while(lightMember)
					{
						Light *light = lightMember->Get();
						if(light->CanRender(renderer, camera))
							light->Render(renderer, camera);
						
						lightMember = lightMember->GetNext();
					}
					
					//Submit all drawables for rendering
					IntrusiveList<SceneNode>::Member *nodeMember = _renderNodes.GetHead();
					while(nodeMember)
					{
						SceneNode *node = nodeMember->Get();
						if(node->CanRender(renderer, camera))
							node->Render(renderer, camera);
						
						nodeMember = nodeMember->GetNext();
					}
				});

				cameraMember = cameraMember->GetNext();
			}
		}

		DidRender(renderer);
	}

	void SceneBasic::AddRenderNode(SceneNode *node)
	{
		int32 renderPriority = node->GetRenderPriority();
		if(!_renderNodes.GetHead() || _renderNodes.GetHead()->Get()->GetRenderPriority() >= renderPriority)
		{
			_renderNodes.PushFront(node->_sceneRenderEntry);
		}
		else if(!_renderNodes.GetTail() || _renderNodes.GetTail()->Get()->GetRenderPriority() <= renderPriority)
		{
			_renderNodes.PushBack(node->_sceneRenderEntry);
		}
		else
		{
			IntrusiveList<SceneNode>::Member *member = _renderNodes.GetHead();
			while(member->Get()->GetRenderPriority() > renderPriority)
			{
				member = member->GetNext();
			}
			
			_renderNodes.InsertAfter(node->_sceneRenderEntry, member);
		}
	}

	void SceneBasic::RemoveRenderNode(SceneNode *node)
	{
		_renderNodes.Erase(node->_sceneRenderEntry);
	}

	void SceneBasic::AddNode(SceneNode *node)
	{
		//Remove from deletion list if scheduled for deletion if the scene didn't change.
		if(node->GetSceneInfo() && node->GetSceneInfo()->GetScene() == this && _nodesToRemove->ContainsObject(node))
		{
			_nodesToRemove->Lock();
			_nodesToRemove->RemoveObject(node);
			_nodesToRemove->Unlock();
			return;
		}
		
		RN_ASSERT(node->GetSceneInfo() == nullptr, "AddNode() must be called on a Node not owned by a scene");
    
		if(node->IsKindOfClass(Camera::GetMetaClass()))
		{
			Camera *camera = static_cast<Camera *>(node);
			_cameras.PushFront(camera->_cameraSceneEntry);
		}
		else if(node->IsKindOfClass(Light::GetMetaClass()))
		{
			Light *light = static_cast<Light *>(node);
			_lights.PushFront(light->_lightSceneEntry);
		}
		else
		{
			AddRenderNode(node);
		}
		
        //PushFront to prevent race condition with scene iterating over the nodes.
		_updateNodes[static_cast<size_t>(node->GetUpdatePriority())].PushFront(node->_sceneUpdateEntry);
        
		node->Retain();
		SceneInfo *sceneInfo = new SceneInfo(this);
		node->UpdateSceneInfo(sceneInfo->Autorelease());
	}
	
	void SceneBasic::RemoveNode(SceneNode *node)
	{
		RN_ASSERT(node->GetSceneInfo() && node->GetSceneInfo()->GetScene() == this, "RemoveNode() must be called on a Node owned by the scene");
		
		_nodesToRemove->Lock();
		_nodesToRemove->AddObject(node);
		_nodesToRemove->Unlock();
	}
}
