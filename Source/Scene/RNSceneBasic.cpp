//
//  RNScene.cpp
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneBasic.h"
#include "../Debug/RNLogger.h"
#include "../Math/RNRandom.h"
#include "../Objects/RNAutoreleasePool.h"
#include "../Rendering/RNMesh.h"
#include "../Rendering/RNModel.h"
#include "../Scene/RNEntity.h"
#include "../Scene/RNLightClusterSceneAttachment.h"
#include "../Scene/RNShadowSceneAttachment.h"
#include "../Threads/RNWorkGroup.h"
#include "../Threads/RNWorkQueue.h"

#define kRNSceneUpdateBatchSize 8192 //1024

namespace RN
{
	RNDefineMeta(SceneBasic, Scene)
	RNDefineMeta(SceneBasicInfo, SceneInfo)

	SceneBasic::OcclusionCullingParameters::OcclusionCullingParameters(uint16 textureWidth, uint16 textureHeight, size_t maxOccluders, uint16 frameCount, float jitter) :
		textureWidth(textureWidth > 0 ? textureWidth : 1),
		textureHeight(textureHeight > 0 ? textureHeight : 1),
		maxOccluders(maxOccluders),
		frameCount(frameCount),
		jitter(jitter)
	{}

	SceneBasicInfo::SceneBasicInfo(Scene *scene) :
		SceneInfo(scene),
		occludedFrameCounter(0),
		occluderSize(0.0f),
		occluderDistance(0.0f),
		isActiveOccluder(false),
		isVisibleOccluder(false),
		isCachedOccluder(false)
	{
		RN_PROFILE_SCOPE();
	}

	SceneBasic::SceneBasic(const OcclusionCullingParameters &occlusionCullingParameters) :
		_nodesToRemove(new Array()), _occlusionCullingParameters(occlusionCullingParameters), _currentFrameCount(0)
	{
		RN_PROFILE_SCOPE();
		_occlusionCuller = new OcclusionCuller(_occlusionCullingParameters.textureWidth, _occlusionCullingParameters.textureHeight);

		LightClusterSceneAttachment::AttachToScene(this);
		ShadowSceneAttachment::AttachToScene(this);
	}

	SceneBasic::~SceneBasic()
	{
		RN_PROFILE_SCOPE();
		PrepareForShutdown();
		_nodesToRemove->Release();
		delete _occlusionCuller;
	}

	void SceneBasic::Update(float delta)
	{
		RN_PROFILE_SCOPE();
		WillUpdate(delta);

		WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default);

		for(size_t i = 0; i < 4; i++)
		{
			RN_PROFILE_SCOPE_N("Update by Priority");
			WorkGroup *group = nullptr;
			IntrusiveList<SceneNode>::Member *member = _updateNodes[i].GetHead();
			IntrusiveList<SceneNode>::Member *first = member;

			size_t count = 0;

			while(member)
			{
				if(count == kRNSceneUpdateBatchSize)
				{
					if(!group) group = new WorkGroup();
					group->Perform(queue, [&, member, first] {
						RN_PROFILE_SCOPE_N("Update Batch");

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
				//				group->Perform(queue, [&, member, first] {

				RN_PROFILE_SCOPE_N("Update Last Batch");

				AutoreleasePool pool;
				auto iterator = first;

				while(iterator != member)
				{
					SceneNode *node = iterator->Get();
					UpdateNode(node, delta);
					iterator = iterator->GetNext();
				}

				//				});
			}

			if(group)
			{
				group->Wait();
				group->Release();
			}
		}

		Scene::Update(delta);
		DidUpdate(delta);

		FlushDeletionQueue();
	}

	void SceneBasic::PrepareForShutdown()
	{
		Scene::PrepareForShutdown();
		RemoveAllNodes();
		FlushDeletionQueue();
	}

	void SceneBasic::RemoveAllNodes()
	{
		Array *nodes = new Array();
		auto addNode = [&](SceneNode *node) {
			if(!nodes->ContainsObject(node))
				nodes->AddObject(node);
		};

		for(IntrusiveList<SceneNode>::Member *member = _renderNodes.GetHead(); member; member = member->GetNext())
			addNode(member->Get());

		for(IntrusiveList<Light>::Member *member = _lights.GetHead(); member; member = member->GetNext())
			addNode(member->Get());

		for(IntrusiveList<Camera>::Member *member = _cameras.GetHead(); member; member = member->GetNext())
			addNode(member->Get());

		nodes->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {
			RemoveNode(node);
		});
		nodes->Release();
	}

	void SceneBasic::FlushDeletionQueue()
	{
		RN_PROFILE_SCOPE();
		_nodesToRemove->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {
			RemoveRenderNode(node);

			if(node->GetUpdatePriority() != SceneNode::UpdatePriority::UpdateNever)
			{
				_updateNodes[static_cast<size_t>(node->GetUpdatePriority())].Erase(node->_sceneUpdateEntry);
			}

			node->UpdateSceneInfo(nullptr);
			node->_scheduledForRemovalFromScene = false; //Make sure it can be added and removed again, if the object doesn't actually get deleted here!
			node->Autorelease(); //Autorelease here causes the constructors to be called later (otherwise RemoveAllObjects below would do it), this will cause objects removed from the scene in the destructor to be removed in the next frame, working around some issue I've been having otherwise
			//TODO: Somehow make objects removed in the destructor also be deleted here
		});

		_nodesToRemove->RemoveAllObjects();
	}

	bool SceneBasic::IsOccluderCacheCandidate(SceneNode *node) const
	{
		return node->GetRenderPriority() < SceneNode::RenderPriority::RenderSky && node->HasFlags(SceneNode::Flags::Occluder);
	}

	void SceneBasic::AddCachedOccluderNode(SceneNode *node)
	{
		if(!IsOccluderCacheCandidate(node)) return;

		SceneBasicInfo *sceneInfo = static_cast<SceneBasicInfo *>(node->GetSceneInfo());
		if(!sceneInfo || sceneInfo->isCachedOccluder) return;

		sceneInfo->isCachedOccluder = true;
		_occluderNodes.push_back(node);
	}

	void SceneBasic::RemoveCachedOccluderNode(SceneNode *node)
	{
		SceneBasicInfo *sceneInfo = static_cast<SceneBasicInfo *>(node->GetSceneInfo());
		if(sceneInfo)
		{
			sceneInfo->isCachedOccluder = false;
			sceneInfo->isActiveOccluder = false;
		}

		for(auto iterator = _occluderNodes.begin(); iterator != _occluderNodes.end(); iterator++)
		{
			if(*iterator == node)
			{
				*iterator = _occluderNodes.back();
				_occluderNodes.pop_back();
				break;
			}
		}
	}


	void SceneBasic::Render(Renderer *renderer)
	{
		RN_PROFILE_SCOPE();
		WillRender(renderer);

		//Run camera PostUpdate once for each camera
		IntrusiveList<Camera>::Member *cameraMember = _cameras.GetHead();
		while(cameraMember)
		{
			Camera *camera = cameraMember->Get();
			camera->PostUpdate();
			Camera *lodCamera = camera->GetLODCamera();
			if(lodCamera != camera) lodCamera->PostUpdate();
			cameraMember = cameraMember->GetNext();
		}

		for(int cameraPriority = 0; cameraPriority < 3; cameraPriority++)
		{
			cameraMember = _cameras.GetHead();
			while(cameraMember)
			{
				RN_PROFILE_SCOPE();
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

				const PositionType &renderOrigin = camera->GetRenderOrigin();
				auto getRenderBounds = [&renderOrigin](const SceneNode *node) {
					AABB bounds = node->GetBoundingBox();
					bounds.position -= renderOrigin;
					return bounds;
				};

				std::vector<SceneNode *> occluders;
				std::vector<SceneNode *> sceneNodesToRender;
				size_t firstTransparentIndex = 0;
				size_t lastTransparentIndex = 0;

				occluders.reserve(_occluderNodes.size());
				sceneNodesToRender.reserve(_renderNodes.GetCount());

				IntrusiveList<SceneNode>::Member *firstNodeMember = camera->_firstNodeMember ? camera->_firstNodeMember : _renderNodes.GetHead();
				IntrusiveList<SceneNode>::Member *nodeMember = firstNodeMember;
				//TODO: Occlusion culling is skipped for cameras rendering a later slice of the render list. The cached occluder list is global and can contain occluders before firstNodeMember.
				if(firstNodeMember == _renderNodes.GetHead() && _occlusionCullingParameters.maxOccluders > 0 && !(camera->GetFlags() & Camera::Flags::NoOcclusionCulling) && !camera->GetRenderNodes())
				{
					RN_PROFILE_SCOPE_N("Collect Occluders");
					const PositionType cameraWorldPosition = camera->GetWorldPosition();
					for(size_t i = 0; i < _occluderNodes.size();)
					{
						SceneNode *node = _occluderNodes[i];
						SceneBasicInfo *sceneInfo = static_cast<SceneBasicInfo *>(node->GetSceneInfo());
						if(!sceneInfo || !IsOccluderCacheCandidate(node))
						{
							RemoveCachedOccluderNode(node);
							continue;
						}

						sceneInfo->isActiveOccluder = false;
						if(node->CanRender(renderer, camera) && !node->GetBoundingBox().Contains(cameraWorldPosition))
						{
							sceneInfo->occluderDistance = std::max<float>(node->GetWorldPosition().GetSquaredDistance(cameraWorldPosition), 1.0f);
							sceneInfo->occluderSize = node->GetBoundingSphere().radius * node->GetBoundingSphere().radius / sceneInfo->occluderDistance;
							occluders.push_back(node);
						}

						i++;
					}

					nodeMember = firstNodeMember;
				}

				//Do occlusion culling if there are 1 or more occluders!
				if(occluders.size() > 0)
				{
					RN_PROFILE_SCOPE_N("Collect Entities with Occlusion Culling");
					{
						RN_PROFILE_SCOPE_N("Find Biggest Occluders");

						//Sort occluders by approximated size on the screen
						if(occluders.size() > _occlusionCullingParameters.maxOccluders)
						{
							std::nth_element(occluders.begin(), occluders.begin() + _occlusionCullingParameters.maxOccluders, occluders.end(), [](SceneNode *a, SceneNode *b) {
								SceneBasicInfo *sceneInfoA = static_cast<SceneBasicInfo *>(a->GetSceneInfo());
								SceneBasicInfo *sceneInfoB = static_cast<SceneBasicInfo *>(b->GetSceneInfo());
								return sceneInfoA->occluderSize > sceneInfoB->occluderSize;
							});

							occluders.resize(_occlusionCullingParameters.maxOccluders);
						}

						//Sort remaining occluders front to back
						std::sort(occluders.begin(), occluders.end(), [](SceneNode *a, SceneNode *b) {
							SceneBasicInfo *sceneInfoA = static_cast<SceneBasicInfo *>(a->GetSceneInfo());
							SceneBasicInfo *sceneInfoB = static_cast<SceneBasicInfo *>(b->GetSceneInfo());
							return sceneInfoA->occluderDistance < sceneInfoB->occluderDistance;
						});
					}

					//Clear occlusion depth map
					_occlusionCuller->Clear();

					Vector2 screenPixelSize = Vector2(1.0f / camera->GetRenderPass()->GetFrame().width, 1.0f / camera->GetRenderPass()->GetFrame().height);

					float occlusionJitter = _occlusionCullingParameters.jitter;
					Vector3 randomCameraOffset = RandomNumberGenerator::GetSharedGenerator()->GetRandomVector3Range(RN::Vector3(-occlusionJitter, -occlusionJitter, 0.0f), RN::Vector3(occlusionJitter, occlusionJitter, 0.0f));
					Matrix matJitter = Matrix::WithTranslation(randomCameraOffset);
					Matrix matViewProj = camera->GetProjectionMatrix() * matJitter * camera->GetViewMatrix();
					const Array *multiviewCameras = camera->GetMultiviewCameras();
					if(multiviewCameras && multiviewCameras->GetCount() > 0)
					{
						size_t multiviewIndex = _currentFrameCount % multiviewCameras->GetCount();
						RN::Camera *multiviewCamera = multiviewCameras->GetObjectAtIndex<RN::Camera>(multiviewIndex);
						matViewProj = multiviewCamera->GetProjectionMatrix() * matJitter * multiviewCamera->GetViewMatrix();
					}

					{
						RN_PROFILE_SCOPE_N("Render Occluder Depth");

						//Render occluders to depth buffer first (first test if the bounding box is visible at all)
						for(SceneNode *node : occluders)
						{
							bool testResult = _occlusionCuller->TestBoundingBox(matViewProj, getRenderBounds(node), screenPixelSize);
							SceneBasicInfo *sceneInfo = static_cast<SceneBasicInfo *>(node->GetSceneInfo());
							sceneInfo->isActiveOccluder = true;
							if(!testResult && sceneInfo->occludedFrameCounter < 1000)
							{
								sceneInfo->occludedFrameCounter += 1;
							}
							if(testResult || sceneInfo->occludedFrameCounter < _occlusionCullingParameters.frameCount)
							{
								if(testResult)
								{
									sceneInfo->occludedFrameCounter = 0;
								}
							}

							if(testResult)
							{
								RN::Entity *entity = node->Downcast<Entity>();
								if(entity)
								{
									Matrix matModelViewProj = matViewProj * node->GetWorldTransform(renderOrigin);

									Model *model = entity->GetModel();
									RN::Model::LODStage *lodStage = model->GetLODStage(0);
									for(int i = 0; i < lodStage->GetCount(); i++)
									{
										RN::Mesh *mesh = lodStage->GetMeshAtIndex(i);
										_occlusionCuller->RasterizeMesh(matModelViewProj, mesh);
									}
								}
							}
						}
					}

					{
						RN_PROFILE_SCOPE_N("Test Objects");

						//Test all visible objects against depth buffer
						while(nodeMember)
						{
							SceneNode *node = nodeMember->Get();
							nodeMember = nodeMember->GetNext();
							if(!node->CanRender(renderer, camera)) continue;
							if(node->GetRenderPriority() >= SceneNode::RenderSky)
							{
								if(node->GetRenderPriority() == SceneNode::RenderTransparent)
								{
									if(firstTransparentIndex == 0)
									{
										firstTransparentIndex = sceneNodesToRender.size();
									}
									lastTransparentIndex = sceneNodesToRender.size();
								}
								sceneNodesToRender.push_back(node);
								continue;
							}

							SceneNode::Flags nodeFlags = node->GetFlags();
							SceneBasicInfo *sceneInfo = static_cast<SceneBasicInfo *>(node->GetSceneInfo());
							if(nodeFlags & SceneNode::Flags::Occluder)
							{
								AddCachedOccluderNode(node);
							}

							if(nodeFlags & SceneNode::Flags::NoCulling)
							{
								sceneInfo->occludedFrameCounter = 0;
								sceneNodesToRender.push_back(node);
								continue;
							}

							if(nodeFlags & SceneNode::Flags::Occluder)
							{
								if(sceneInfo->isActiveOccluder && sceneInfo->occludedFrameCounter < _occlusionCullingParameters.frameCount)
								{
									sceneNodesToRender.push_back(node);
									continue;
								}
							}

							bool testResult = _occlusionCuller->TestBoundingBox(matViewProj, getRenderBounds(node), screenPixelSize);
							if(!testResult && sceneInfo->occludedFrameCounter < 1000)
							{
								sceneInfo->occludedFrameCounter += 1;
							}
							if(testResult || sceneInfo->occludedFrameCounter < _occlusionCullingParameters.frameCount)
							{
								if(testResult)
								{
									sceneInfo->occludedFrameCounter = 0;
								}

								sceneNodesToRender.push_back(node);
							}
						}
					}
				}
				else if(camera->GetFlags() & Camera::Flags::UseUIFastPath)
				{
					RN_PROFILE_SCOPE_N("Collect UI Entities");
					while(nodeMember)
					{
						SceneNode *node = nodeMember->Get();
						if((node->GetRenderGroup() & camera->GetRenderGroup()) == 0 || node->HasFlags(RN::SceneNode::Flags::Hidden))
						{
							nodeMember = nodeMember->GetNext();
							continue;
						}
						if(node->GetMaximumRenderDistance() > 0.0f && !node->CanRender(renderer, camera))
						{
							nodeMember = nodeMember->GetNext();
							continue;
						}

						if(node->GetRenderPriority() == SceneNode::RenderTransparent)
						{
							if(firstTransparentIndex == 0)
							{
								firstTransparentIndex = sceneNodesToRender.size();
							}
							lastTransparentIndex = sceneNodesToRender.size();
						}
						sceneNodesToRender.push_back(node);

						nodeMember = nodeMember->GetNext();
					}
				}
				else
				{
					RN_PROFILE_SCOPE_N("Collect Entities");
					if(camera->GetRenderNodes())
					{
						camera->GetRenderNodes()->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {
							//if(node->CanRender(renderer, camera))
							{
								if(node->GetRenderPriority() == SceneNode::RenderTransparent)
								{
									if(firstTransparentIndex == 0)
									{
										firstTransparentIndex = sceneNodesToRender.size();
									}
									lastTransparentIndex = sceneNodesToRender.size();
								}
								sceneNodesToRender.push_back(node);
							}
						});
					}
					else
					{
						while(nodeMember)
						{
							SceneNode *node = nodeMember->Get();
							if(node->CanRender(renderer, camera))
							{
								if(node->GetFlags() & SceneNode::Flags::Occluder)
								{
									AddCachedOccluderNode(node);
								}

								if(node->GetRenderPriority() == SceneNode::RenderTransparent)
								{
									if(firstTransparentIndex == 0)
									{
										firstTransparentIndex = sceneNodesToRender.size();
									}
									lastTransparentIndex = sceneNodesToRender.size();
								}
								sceneNodesToRender.push_back(node);
							}

							nodeMember = nodeMember->GetNext();
						}
					}
				}

				if(camera->GetFlags() & Camera::Flags::SortFrontToBack)
				{
					RN_PROFILE_SCOPE_N("Sort Opaque");
					const PositionType cameraWorldPosition = camera->GetWorldPosition();
					std::sort(sceneNodesToRender.begin(), sceneNodesToRender.end(), [cameraWorldPosition](SceneNode *a, SceneNode *b) {
						if(a->GetRenderPriority() == b->GetRenderPriority() && b->GetRenderPriority() < SceneNode::RenderSky)
						{
							return a->GetWorldPosition().GetSquaredDistance(cameraWorldPosition) < b->GetWorldPosition().GetSquaredDistance(cameraWorldPosition);
						}
						return a->GetRenderPriority() < b->GetRenderPriority();
					});
				}

				if(camera->GetFlags() & Camera::Flags::SortTransparentBackToFront && firstTransparentIndex < lastTransparentIndex)
				{
					RN_PROFILE_SCOPE_N("Sort Transparent");
					const PositionType cameraWorldPosition = camera->GetWorldPosition();
					std::sort(sceneNodesToRender.begin() + firstTransparentIndex, sceneNodesToRender.begin() + lastTransparentIndex + 1, [cameraWorldPosition](SceneNode *a, SceneNode *b) {
						return a->GetWorldPosition().GetSquaredDistance(cameraWorldPosition) > b->GetWorldPosition().GetSquaredDistance(cameraWorldPosition);
					});
				}

				//RNInfo("Number of objects: " << sceneNodesToRender.size());
				renderer->SubmitCamera(camera, [&] {
					RN_PROFILE_SCOPE_N("Submit Drawables");
					//TODO: Add back some multithreading while not breaking the priorities.

					//Submit lights first
					IntrusiveList<Light>::Member *lightMember = _lights.GetHead();
					std::vector<Light *> visibleLights;
					while(lightMember)
					{
						Light *light = lightMember->Get();
						if(light->CanRender(renderer, camera))
						{
							visibleLights.push_back(light);
							light->Render(renderer, camera);
						}

						lightMember = lightMember->GetNext();
					}

					SceneCameraPassContext cameraPassContext(visibleLights);
					SubmitCameraPassAttachmentSnapshots(renderer, camera, cameraPassContext);

					//Submit all drawables for rendering
					for(SceneNode *node : sceneNodesToRender)
					{
						node->PrepareForRenderIfNeeded();
						node->Render(renderer, camera);
					}
				});

				cameraMember = cameraMember->GetNext();
			}
		}

		_currentFrameCount += 1;
		_currentFrameCount %= 10000;

		DidRender(renderer);
	}

	void SceneBasic::AddRenderNode(SceneNode *node)
	{
		RN_PROFILE_SCOPE();

		if(node->IsKindOfClass(Camera::GetMetaClass()))
		{
			Camera *camera = static_cast<Camera *>(node);
			_cameras.PushFront(camera->_cameraSceneEntry);
			return;
		}

		if(node->IsKindOfClass(Light::GetMetaClass()))
		{
			Light *light = static_cast<Light *>(node);
			_lights.PushFront(light->_lightSceneEntry);
			return;
		}

		Lock();
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
			IntrusiveList<SceneNode>::Member *member = _renderNodes.GetTail();
			while(member->Get()->GetRenderPriority() > renderPriority)
			{
				member = member->GetPrevious();
			}

			_renderNodes.InsertAfter(node->_sceneRenderEntry, member);
		}

		Unlock();
		AddCachedOccluderNode(node);
	}

	void SceneBasic::RemoveRenderNode(SceneNode *node)
	{
		RN_PROFILE_SCOPE();

		if(node->IsKindOfClass(Camera::GetMetaClass()))
		{
			Camera *camera = static_cast<Camera *>(node);
			_cameras.Erase(camera->_cameraSceneEntry);
			return;
		}

		if(node->IsKindOfClass(Light::GetMetaClass()))
		{
			Light *light = static_cast<Light *>(node);
			_lights.Erase(light->_lightSceneEntry);
			return;
		}

		RemoveCachedOccluderNode(node);
		_renderNodes.Erase(node->_sceneRenderEntry);
	}

	void SceneBasic::AddNode(SceneNode *node)
	{
		RN_PROFILE_SCOPE();
		if(_isPreparedForShutdown) return;

		//Remove from deletion list if scheduled for deletion if the scene didn't change.
		if(node->GetSceneInfo() && node->GetSceneInfo()->GetScene() == this && node->_scheduledForRemovalFromScene)
		{
			_nodesToRemove->Lock();
			_nodesToRemove->RemoveObject(node);
			_nodesToRemove->Unlock();

			//Remove and insert at the correct position, respecting the render priority.
			Lock();
			SceneInfo *sceneInfo = node->GetSceneInfo();
			sceneInfo->Retain();
			RemoveRenderNode(node);
			node->UpdateSceneInfo(nullptr);
			Unlock();

			node->UpdateSceneInfo(sceneInfo);
			AddRenderNode(node);
			sceneInfo->Release();

			node->_scheduledForRemovalFromScene = false; //Do this last, so scene change callbacks can check if this is just readding or completely new

			return;
		}

		RN_ASSERT(node->GetSceneInfo() == nullptr, "AddNode() must be called on a Node not owned by a scene");

		node->Retain();
		SceneBasicInfo *sceneInfo = new SceneBasicInfo(this);
		node->UpdateSceneInfo(sceneInfo);
		sceneInfo->Release();

		AddRenderNode(node);

		//Lock to prevent race condition of multiple threads adding nodes at the same time
		Lock();
		//PushFront to prevent race condition with scene iterating over the nodes.
		if(node->GetUpdatePriority() != SceneNode::UpdatePriority::UpdateNever)
		{
			_updateNodes[static_cast<size_t>(node->GetUpdatePriority())].PushFront(node->_sceneUpdateEntry);
		}
		Unlock();
	}

	void SceneBasic::RemoveNode(SceneNode *node)
	{
		RN_PROFILE_SCOPE();
		SceneInfo *sceneInfo = node->GetSceneInfo();
		if(!sceneInfo && _isPreparedForShutdown) return;
		RN_ASSERT(sceneInfo && sceneInfo->GetScene() == this, "RemoveNode() must be called on a Node owned by the scene");

		if(node->_scheduledForRemovalFromScene) return; //Already queued for removal

		_nodesToRemove->Lock();
		_nodesToRemove->AddObject(node);
		_nodesToRemove->Unlock();
		node->_scheduledForRemovalFromScene = true;
	}
} // namespace RN
