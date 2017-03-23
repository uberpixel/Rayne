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

	Scene::Scene()
	{}
	Scene::~Scene()
	{}

	void Scene::Update(float delta)
	{
		WillUpdate(delta);

		WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default);

		std::vector<SceneNode *> temp;
		temp.reserve(kRNSceneUpdateBatchSize);

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

		DidUpdate(delta);
	}

	void Scene::Render(Renderer *renderer)
	{
		WillRender(renderer);

#if kRNSceneUseRenderPool
		WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default);

		IntrusiveList<Camera>::Member *member = _cameras.GetHead();
		while(member)
		{
			Camera *camera = member->Get();
			camera->PostUpdate(renderer);

			renderer->SubmitCamera(camera, [&] {

				WorkGroup *group = new WorkGroup();

				for(size_t i = 0; i < 3; i ++)
				{
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

									if(node->CanRender(renderer, camera))
										node->Render(renderer, camera);

									iterator = iterator->GetNext();
								}

							});

							first = member;
							count = 0;
						}

						member = member->GetNext();
						count ++;
					}

					if(first != member)
					{
						group->Perform(queue, [&, member, first] {

							AutoreleasePool pool;
							auto iterator = first;

							while(iterator != member)
							{
								SceneNode *node = iterator->Get();

								if(node->CanRender(renderer, camera))
									node->Render(renderer, camera);

								iterator = iterator->GetNext();
							}

						});
					}
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

		DidRender(renderer);
	}

	void Scene::AddNode(SceneNode *node)
	{
		RN_ASSERT(node->_scene == nullptr, "AddNode() must be called on a Node not owned by the scene");


		if(node->IsKindOfClass(Camera::GetMetaClass()))
		{
			Camera *camera = static_cast<Camera *>(node);

			_cameras.PushBack(camera->_cameraSceneEntry);

			camera->_scene = this;
			camera->Retain();

			return;
		}

		_nodes[static_cast<size_t>(node->GetPriority())].PushBack(node->_sceneEntry);

		node->Retain();
		node->UpdateScene(this);
	}

	void Scene::RemoveNode(SceneNode *node)
	{
		RN_ASSERT(node->_scene == this, "RemoveNode() must be called on a Node owned by the scene");

		_nodes[static_cast<size_t>(node->GetPriority())].Erase(node->_sceneEntry);

		node->UpdateScene(nullptr);
		node->Autorelease();
	}

	void Scene::WillBecomeActive()
	{}
	void Scene::DidBecomeActive()
	{}

	void Scene::WillResignActive()
	{}
	void Scene::DidResignActive()
	{}

	void Scene::WillUpdate(float delta)
	{}
	void Scene::DidUpdate(float delta)
	{}
	void Scene::WillRender(Renderer *renderer)
	{}
	void Scene::DidRender(Renderer *renderer)
	{}
}
