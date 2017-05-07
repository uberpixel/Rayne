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

	Scene::Scene() : _attachments(nullptr), _lights(new Array())
	{}
	Scene::~Scene()
	{
		if(_attachments)
			_attachments->Release();

		_lights->Release();
	}

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

		//Update scene attachments
		if(_attachments)
		{
			_attachments->Enumerate<SceneAttachment>([delta](SceneAttachment *attachment, size_t index, bool &stop) {
				attachment->Update(delta);
			});
		}

		DidUpdate(delta);
	}

	void Scene::Render(Renderer *renderer)
	{
		WillRender(renderer);

#if kRNSceneUseRenderPool
		WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default);

		//TODO: Change order back to normal, currently just swapped to have shadow depth rendered before the main camera
		IntrusiveList<Camera>::Member *member = _cameras.GetTail();
		while(member)
		{
			Camera *camera = member->Get();
			camera->PostUpdate(renderer);

			if (camera->GetFlags() & Camera::Flags::NoRender)
			{
				member = member->GetPrevious(); //TODO: Switch back to GetNext()...
				continue;
			}

			renderer->SubmitCamera(camera, [&] {

				//TODO: Handle lights to be rendered before other things differently...
				_lights->Enumerate<Light>([renderer, camera](Light *light, size_t index, bool &stop){
					if(light->CanRender(renderer, camera))
						light->Render(renderer, camera);
				});

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

									if(!node->IsKindOfClass(Light::GetMetaClass()))	//TODO: Shitty...
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

								if(!node->IsKindOfClass(Light::GetMetaClass()))	//TODO: Shitty...
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

			member = member->GetPrevious();	//TODO: Switch back to GetNext()...
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
		}

		//TODO: Handle lights to be rendered before other things differently...
		if(node->IsKindOfClass(Light::GetMetaClass()))
		{
			Light *light = static_cast<Light *>(node);
			_lights->AddObject(light);
		}

		_nodes[static_cast<size_t>(node->GetPriority())].PushBack(node->_sceneEntry);

		node->Retain();
		node->UpdateScene(this);
	}

	void Scene::RemoveNode(SceneNode *node)
	{
		RN_ASSERT(node->_scene == this, "RemoveNode() must be called on a Node owned by the scene");

		//TODO: Handle lights to be rendered before other things differently...
		if(node->IsKindOfClass(Light::GetMetaClass()))
		{
			Light *light = static_cast<Light *>(node);
			_lights->RemoveObject(light);
			return;
		}

		_nodes[static_cast<size_t>(node->GetPriority())].Erase(node->_sceneEntry);

		node->UpdateScene(nullptr);
		node->Autorelease();
	}

	void Scene::AddAttachment(SceneAttachment *attachment)
	{
		RN_ASSERT(attachment->_scene == nullptr, "AddAttachment() must be called on an Attachment not owned by the scene");

		if(!_attachments)
			_attachments = new Array();

		_attachments->AddObject(attachment);
		attachment->_scene = this;
	}

	void Scene::RemoveAttachment(SceneAttachment *attachment)
	{
		RN_ASSERT(attachment->_scene == this, "RemoveAttachment() must be called on an Attachment owned by the scene");

		_attachments->RemoveObject(attachment);
		attachment->_scene = nullptr;
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
