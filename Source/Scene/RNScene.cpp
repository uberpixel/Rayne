//
//  RNScene.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNScene.h"
#include "../Debug/RNLogger.h"
#include "../Threads/RNWorkQueue.h"
#include "../Threads/RNWorkGroup.h"
#include "../Objects/RNAutoreleasePool.h"

#define kRNSceneUpdateBatchSize 64
#define kRNSceneRenderBatchSize 32
#define kRNSceneUseRenderPool 1

namespace RN
{
	RNDefineMeta(Scene, Object)

	void Scene::Update(float delta)
	{
		WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default);

		std::vector<SceneNode *> temp;
		temp.reserve(kRNSceneUpdateBatchSize);

		for(size_t i = 0; i < 3; i ++)
		{
			WorkGroup *group = new WorkGroup();

			IntrusiveList<SceneNode>::Member *member = _nodes[i].GetHead();
			while(member)
			{
				SceneNode *node = member->Get();
				temp.push_back(node);

				if(temp.size() == kRNSceneUpdateBatchSize)
				{
					group->Perform(queue, [&, temp] {

						AutoreleasePool pool;

						for(SceneNode *node : temp)
						{
							node->Update(delta);
							node->UpdateInternalData();
						}

					});

					temp.clear();
				}

				member = member->GetNext();
			}

			if(temp.size() > 0)
			{
				group->Perform(queue, [&, temp] {

					AutoreleasePool pool;

					for(SceneNode *node : temp)
					{
						node->Update(delta);
						node->UpdateInternalData();
					}

				});
			}

			group->Wait();
			group->Release();
		}
	}

	void Scene::Render(Renderer *renderer)
	{
#if kRNSceneUseRenderPool
		WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default);

		IntrusiveList<Camera>::Member *member = _cameras.GetHead();
		while(member)
		{
			Camera *camera = member->Get();
			camera->PostUpdate(renderer);

			WorkGroup *group = new WorkGroup();

			renderer->RenderIntoCamera(camera, [&] {

				std::vector<SceneNode *> temp;
				temp.reserve(kRNSceneRenderBatchSize);

				for(size_t i = 0; i < 3; i++)
				{
					IntrusiveList<SceneNode>::Member *member = _nodes[i].GetHead();
					while(member)
					{
						SceneNode *node = member->Get();
						temp.push_back(node);

						if(temp.size() == kRNSceneRenderBatchSize)
						{
							group->Perform(queue, [&, temp] {

								AutoreleasePool pool;

								for(SceneNode *node : temp)
								{
									if(node->CanRender(renderer, camera))
										node->Render(renderer, camera);
								}

							});

							temp.clear();
						}

						member = member->GetNext();
					}
				}

				if(temp.size() > 0)
				{
					group->Perform(queue, [&, temp] {

						AutoreleasePool pool;

						for(SceneNode *node : temp)
						{
							node->Render(renderer, camera);
						}

					});
				}

				group->Wait();
				group->Release();

			});

			member = member->GetNext();
		}
#else
		IntrusiveList<Camera>::Member *member = _cameras.GetHead();
		while(member)
		{
			Camera *camera = member->Get();
			WorkGroup *group = new WorkGroup();

			renderer->BeginCamera(camera);

			for(size_t i = 0; i < 3; i ++)
			{
				IntrusiveList<SceneNode>::Member *member = _nodes[i].GetHead();
				while(member)
				{
					SceneNode *node = member->Get();
					node->Render(renderer, camera);

					member = member->GetNext();
				}
			}

			renderer->EndCamera();

			member = member->GetNext();
		}
#endif
	}

	void Scene::AddNode(SceneNode *node)
	{
		if(node->_scene)
			node->_scene->RemoveNode(node);

		if(node->IsKindOfClass(Camera::GetMetaClass()))
		{
			Camera *camera = static_cast<Camera *>(node);

			_cameras.PushBack(camera->_cameraSceneEntry);

			camera->_scene = this;
			camera->Retain();

			return;
		}

		_nodes[static_cast<size_t>(node->GetPriority())].PushBack(node->_sceneEntry);

		node->_scene = this;
		node->Retain();
	}

	void Scene::RemoveNode(SceneNode *node)
	{
		RN_ASSERT(node->_scene == this, "RemoveNode() must be called on a Node owned by the scene");

		_nodes[static_cast<size_t>(node->GetPriority())].Erase(node->_sceneEntry);

		node->_scene = nullptr;
		node->Autorelease();
	}
}
