//
//  RNScene.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNScene.h"
#include "../Threads/RNWorkQueue.h"
#include "../Threads/RNWorkGroup.h"

namespace RN
{
	RNDefineMeta(Scene, Object)

	void Scene::Update(float delta)
	{
		WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default);

		for(size_t i = 0; i < 3; i ++)
		{
			WorkGroup *group = new WorkGroup();

			IntrusiveList<SceneNode>::Member *member = _nodes[i].GetHead();
			while(member)
			{
				SceneNode *node = member->Get();

				group->Perform(queue, [&, node] {
					node->Update(delta);
				});

				member = member->GetNext();
			}

			group->Wait();
			group->Release();
		}
	}

	void Scene::Render(Renderer *renderer)
	{
		IntrusiveList<Camera>::Member *member = _cameras.GetHead();
		while(member)
		{
			Camera *camera = member->Get();

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
