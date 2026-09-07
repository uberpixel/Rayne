//
//  RNSceneWithVisibilityLists.cpp
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneWithVisibilityLists.h"
#include "../Debug/RNLogger.h"
#include "../Objects/RNAutoreleasePool.h"
#include "../Scene/RNLightClusterSceneAttachment.h"
#include "../Scene/RNShadowSceneAttachment.h"
#include "../Threads/RNWorkGroup.h"
#include "../Threads/RNWorkQueue.h"

#define kRNSceneUpdateBatchSize 64

namespace RN
{
	RNDefineMeta(SceneWithVisibilityLists, Scene)
	RNDefineScopedMeta(SceneWithVisibilityLists, Volume, Object)
	RNDefineScopedMeta(SceneWithVisibilityLists, AxisAlignedBoxVolume, SceneWithVisibilityLists::Volume)
	RNDefineMeta(SceneWithVisibilityListsInfo, SceneInfo)

	SceneWithVisibilityListsInfo::SceneWithVisibilityListsInfo(Scene *scene) :
		SceneInfo(scene)
	{
	}

	bool SceneWithVisibilityLists::Volume::ContainsPosition(const PositionType &cameraPosition) const
	{
		return true;
	}

	bool SceneWithVisibilityLists::AxisAlignedBoxVolume::ContainsPosition(const PositionType &cameraPosition) const
	{
		return (cameraPosition.z >= boundsMin.z && cameraPosition.y >= boundsMin.y && cameraPosition.x >= boundsMin.x && cameraPosition.x <= boundsMax.x && cameraPosition.y <= boundsMax.y && cameraPosition.z <= boundsMax.z);
	}

	SceneWithVisibilityLists::SceneWithVisibilityLists() :
		_isAddingVolume(false)
	{
		_volumes = new Array();
		_defaultVolume = new Volume();

		LightClusterSceneAttachment::AttachToScene(this);
		ShadowSceneAttachment::AttachToScene(this);
	}
	SceneWithVisibilityLists::~SceneWithVisibilityLists()
	{
		PrepareForShutdown();
		SafeRelease(_volumes);
		SafeRelease(_defaultVolume);
	}

	void SceneWithVisibilityLists::Update(float delta)
	{
		WillUpdate(delta);

		WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default);

		for(size_t i = 0; i < 4; i++)
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
				count++;
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

		Scene::Update(delta);

		DidUpdate(delta);
	}

	void SceneWithVisibilityLists::RenderVolumeList(Renderer *renderer, Camera *camera, const Volume *volume)
	{
		for(SceneNode *node : volume->nodes)
		{
			if(node->CanRender(renderer, camera))
			{
				node->PrepareForRenderIfNeeded();
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

				if(camera->GetIsMultiviewCamera())
				{
					member = member->GetNext();
					continue;
				}

				camera->PostUpdate();
				Camera *lodCamera = camera->GetLODCamera();
				if(lodCamera != camera) lodCamera->PostUpdate();
				const PositionType cameraPosition = camera->GetWorldPosition();

				IntrusiveList<Light>::Member *lightMember = _lights.GetHead();
				std::vector<Light *> visibleLights;
				while(lightMember)
				{
					Light *light = lightMember->Get();
					if(light->CanRender(renderer, camera))
					{
						visibleLights.push_back(light);
					}
					lightMember = lightMember->GetNext();
				}

				const Volume *volume = nullptr;
				_volumes->Enumerate<Volume>([&](Volume *object, size_t index, bool &stop) {
					if(object->ContainsPosition(cameraPosition))
					{
						volume = object;
						stop = true;
					}
				});

				renderer->SubmitCamera(camera, [&] {
					for(Light *light : visibleLights)
						light->Render(renderer, camera);

					SceneCameraPassContext cameraPassContext(visibleLights);
					SubmitCameraPassAttachmentSnapshots(renderer, camera, cameraPassContext);
					RenderVolumeList(renderer, camera, _defaultVolume);
					if(volume)
					{
						RenderVolumeList(renderer, camera, volume);
					}
				});

				member = member->GetNext();
			}
		}

		DidRender(renderer);
	}

	void SceneWithVisibilityLists::PrepareForShutdown()
	{
		Scene::PrepareForShutdown();
		Array *nodes = new Array();
		auto addNode = [&](SceneNode *node) {
			if(!nodes->ContainsObject(node))
				nodes->AddObject(node);
		};

		_volumes->Enumerate<Volume>([&](Volume *volume, size_t index, bool &stop) {
			for(SceneNode *node : volume->nodes)
				addNode(node);
		});

		nodes->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {
			RemoveNode(node);
		});
		nodes->Release();
	}

	void SceneWithVisibilityLists::AddVolume(Volume *volume)
	{
		_volumes->AddObject(volume);

		_isAddingVolume = true;
		for(SceneNode *node : volume->nodes)
		{
			if(!node->GetSceneInfo())
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
		if(_isPreparedForShutdown) return;

		RN_ASSERT(node->GetSceneInfo() == nullptr, "AddNode() must be called on a Node not owned by a scene");

		if(node->IsKindOfClass(Camera::GetMetaClass()))
		{
			Camera *camera = static_cast<Camera *>(node);
			_cameras.PushBack(camera->_cameraSceneEntry);
		}
		else if(node->IsKindOfClass(Light::GetMetaClass()))
		{
			Light *light = static_cast<Light *>(node);
			_lights.PushBack(light->_lightSceneEntry);
		}

		if(node->GetUpdatePriority() != SceneNode::UpdatePriority::UpdateNever)
		{
			_updateNodes[static_cast<size_t>(node->GetUpdatePriority())].PushBack(node->_sceneUpdateEntry);
		}

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
		SceneInfo *sceneInfo = node->GetSceneInfo();
		if(!sceneInfo && _isPreparedForShutdown) return;
		RN_ASSERT(sceneInfo && sceneInfo->GetScene() == this, "RemoveNode() must be called on a Node owned by the scene");

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

		if(node->GetUpdatePriority() != SceneNode::UpdatePriority::UpdateNever)
		{
			_updateNodes[static_cast<size_t>(node->GetUpdatePriority())].Erase(node->_sceneUpdateEntry);
		}

		SceneWithVisibilityListsInfo *visibilitySceneInfo = sceneInfo->Downcast<SceneWithVisibilityListsInfo>();
		for(Volume *volume : visibilitySceneInfo->volumes)
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
} // namespace RN
