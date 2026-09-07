//
//  RNSceneQuadtree.cpp
//  Rayne
//
//  Copyright 2025 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneQuadtree.h"
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

#define OCCLUSION_FRAMECOUNT 50
#define OCCLUSION_JITTER 0.15f

namespace RN
{
	RNDefineMeta(SceneQuadtree, Scene)
	RNDefineMeta(SceneQuadtreeInfo, SceneInfo)

	SceneQuadtreeInfo::SceneQuadtreeInfo(Scene *scene) :
		SceneInfo(scene), occludedFrameCounter(0), quadtreeNodeIndex(UINT32_MAX)
	{
		RN_PROFILE_SCOPE();
	}

	SceneQuadtree::SceneQuadtree(const AABB &worldBounds, float minNodeSize) :
		_nodesToRemove(new Array()),
		_treeOrigin(worldBounds.position),
		_currentFrameCount(0)
	{
		RN_PROFILE_SCOPE();
		_occlusionCuller = new OcclusionCuller(40, 40);

		LightClusterSceneAttachment::AttachToScene(this);
		ShadowSceneAttachment::AttachToScene(this);

		Vector3 boundsMin = worldBounds.minExtend;
		Vector3 boundsMax = worldBounds.maxExtend;
		boundsMin.y = 0.0f;
		boundsMax.y = 0.0f;
		Vector3 boundsSize = boundsMax - boundsMin;
		float maxBoundsSize = std::max(boundsSize.x, boundsSize.z);
		int treeDepth = std::ceil(std::log2(maxBoundsSize / minNodeSize));
		
		//Generated indices into the node array for where each level begins
		std::vector<uint32> levelOffsets(treeDepth + 1);
		uint32 nodeCount = 0;
		for(int level = 0; level <= treeDepth; level++)
		{
			levelOffsets[level] = nodeCount;
			nodeCount += static_cast<uint32>(std::pow(4, level));
		}
		_treeNodes.resize(nodeCount);

		TreeBounds rootBounds(boundsMin, boundsMin + Vector3(maxBoundsSize, boundsSize.y, maxBoundsSize));
		_treeNodes[0] = { rootBounds, UINT32_MAX, 0, {} };

		for(int level = 0; level < treeDepth; level++)
		{
			uint32 levelStart = levelOffsets[level];
			uint32 nextLevelStart = levelOffsets[level + 1];

			for(uint32 i = 0; i < (nextLevelStart - levelStart); i++)
			{
				uint32 nodeIndex = levelStart + i;
				TreeNode& n = _treeNodes[nodeIndex];

				// Compute firstChild index
				uint32 firstChild = nextLevelStart + i * 4;
				n.firstChild = firstChild;

				// Compute children bounds
				Vector3 parentMidPoint = (n.bounds.min + n.bounds.max) * 0.5f;
				_treeNodes[firstChild + 0] = { TreeBounds(n.bounds.min, Vector3(parentMidPoint.x, n.bounds.max.y, parentMidPoint.z)), UINT32_MAX, 0, {}};
				_treeNodes[firstChild + 1] = { TreeBounds(Vector3(parentMidPoint.x, n.bounds.min.y, n.bounds.min.z), Vector3(n.bounds.max.x, n.bounds.max.y, parentMidPoint.z)), UINT32_MAX, 0, {}};
				_treeNodes[firstChild + 2] = { TreeBounds(Vector3(n.bounds.min.x, n.bounds.min.y, parentMidPoint.z), Vector3(parentMidPoint.x, n.bounds.max.y, n.bounds.max.z)), UINT32_MAX, 0, {}};
				_treeNodes[firstChild + 3] = { TreeBounds(Vector3(parentMidPoint.x, n.bounds.min.y, parentMidPoint.z), n.bounds.max), UINT32_MAX, 0, {}};
			}
		}
		
/*		for(int i = 0; i < nodeCount; i++)
		{
			if(_treeNodes[i].firstChild == UINT32_MAX)
			{
				Mesh *cube = Mesh::WithColoredCube(Vector3(1.0f, 1.0f, 1.0f), RN::Color::Green());
				Model *model = new Model(cube);
				Material *mat = model->GetLODStage(0)->GetMaterialAtIndex(0);
				mat->SetDepthWriteEnabled(false);
				mat->SetDepthMode(RN::DepthMode::Greater);
				mat->SetCullMode(RN::CullMode::None);
				
				Entity *box = new Entity(model);
				box->SetRenderPriority(SceneNode::RenderLate);
				box->SetWorldScale(Vector3(1.0f, 1.0f, 1.0f));
				box->SetWorldPosition(_treeOrigin + (_treeNodes[i].bounds.min + _treeNodes[i].bounds.max) * 0.5f);
				AddNode(box->Autorelease());
			}
		}*/
	}

	SceneQuadtree::~SceneQuadtree()
	{
		RN_PROFILE_SCOPE();
		PrepareForShutdown();
		_nodesToRemove->Release();
		delete _occlusionCuller;
	}

	void SceneQuadtree::Update(float delta)
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
				RN_PROFILE_SCOPE_N("Update Last Batch");

				AutoreleasePool pool;
				auto iterator = first;

				while(iterator != member)
				{
					SceneNode *node = iterator->Get();
					UpdateNode(node, delta);
					iterator = iterator->GetNext();
				}
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

	void SceneQuadtree::PrepareForShutdown()
	{
		Scene::PrepareForShutdown();
		RemoveAllNodes();
		FlushDeletionQueue();
	}

	void SceneQuadtree::RemoveAllNodes()
	{
		Array *nodes = new Array();
		auto addNode = [&](SceneNode *node) {
			if(!nodes->ContainsObject(node))
				nodes->AddObject(node);
		};

		for(TreeNode &treeNode : _treeNodes)
		{
			for(SceneNode *node : treeNode.objects)
				addNode(node);
		}

		for(IntrusiveList<Light>::Member *member = _lights.GetHead(); member; member = member->GetNext())
			addNode(member->Get());

		for(IntrusiveList<Camera>::Member *member = _cameras.GetHead(); member; member = member->GetNext())
			addNode(member->Get());

		nodes->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {
			RemoveNode(node);
		});
		nodes->Release();
	}

	void SceneQuadtree::FlushDeletionQueue()
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

    

	void SceneQuadtree::Render(Renderer *renderer)
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

				std::vector<SceneNode *> sceneNodesToRender;

				/*std::vector<SceneNode *> occluders;
				size_t firstTransparentIndex = 0;
				size_t lastTransparentIndex = 0;

				occluders.reserve(_renderNodes.GetCount());
				sceneNodesToRender.reserve(_renderNodes.GetCount());

				IntrusiveList<SceneNode>::Member *firstNodeMember = camera->_firstNodeMember ? camera->_firstNodeMember : _renderNodes.GetHead();
				IntrusiveList<SceneNode>::Member *nodeMember = firstNodeMember;
				if(!(camera->GetFlags() & Camera::Flags::NoOcclusionCulling) && !camera->GetRenderNodes())
				{
					RN_PROFILE_SCOPE_N("Collect Occluders");
					const PositionType cameraWorldPosition = camera->GetWorldPosition();
					//Collect all occluders
					while(nodeMember)
					{
						SceneNode *node = nodeMember->Get();
						if(node->GetRenderPriority() >= SceneNode::RenderSky) break;

						if(node->HasFlags(SceneNode::Flags::Occluder) && node->CanRender(renderer, camera))
						{
							SceneQuadtreeInfo *sceneInfo = static_cast<SceneQuadtreeInfo *>(node->GetSceneInfo());
							sceneInfo->occluderDistance = std::max<float>(node->GetWorldPosition().GetSquaredDistance(cameraWorldPosition), 1.0f);
							sceneInfo->occluderSize = node->GetBoundingSphere().radius * node->GetBoundingSphere().radius / sceneInfo->occluderDistance;
							sceneInfo->isActiveOccluder = false;
							occluders.push_back(node);
						}

						nodeMember = nodeMember->GetNext();
					}

					nodeMember = firstNodeMember;
				}

				//Do occlusion culling if there are 1 or more occluders!
				if(occluders.size() > 0)
				{
					RN_PROFILE_SCOPE_N("Collect Entities with Occlusion Culling");
					{
						RN_PROFILE_SCOPE_N("Find 30 biggest occluders");

						//Sort occluders by approximated size on the screen
						int clampedCount = std::min(static_cast<int>(occluders.size()), 30);
						std::nth_element(occluders.begin(), occluders.begin() + clampedCount, occluders.end(), [](SceneNode *a, SceneNode *b) {
							SceneQuadtreeInfo *sceneInfoA = static_cast<SceneQuadtreeInfo *>(a->GetSceneInfo());
							SceneQuadtreeInfo *sceneInfoB = static_cast<SceneQuadtreeInfo *>(b->GetSceneInfo());
							return sceneInfoA->occluderSize > sceneInfoB->occluderSize;
						});

						occluders.resize(std::min(static_cast<size_t>(30), occluders.size())); //Only keep the biggest 30 occluders in the list

						//Sort remaining occluders front to back
						std::sort(occluders.begin(), occluders.end(), [](SceneNode *a, SceneNode *b) {
							SceneQuadtreeInfo *sceneInfoA = static_cast<SceneQuadtreeInfo *>(a->GetSceneInfo());
							SceneQuadtreeInfo *sceneInfoB = static_cast<SceneQuadtreeInfo *>(b->GetSceneInfo());
							return sceneInfoA->occluderDistance < sceneInfoB->occluderDistance;
						});
					}

                    //Clear occlusion depth map
                    _occlusionCuller->Clear();

					Vector2 screenPixelSize = Vector2(1.0f / camera->GetRenderPass()->GetFrame().width, 1.0f / camera->GetRenderPass()->GetFrame().height);

					Vector3 randomCameraOffset = RandomNumberGenerator::GetSharedGenerator()->GetRandomVector3Range(RN::Vector3(-OCCLUSION_JITTER, -OCCLUSION_JITTER, 0.0f), RN::Vector3(OCCLUSION_JITTER, OCCLUSION_JITTER, 0.0f));
					Matrix matViewProj = camera->GetProjectionMatrix() * Matrix::WithTranslation(randomCameraOffset) * camera->GetViewMatrix();
					if(camera->GetIsMultiviewCamera())
					{
						size_t multiviewIndex = _currentFrameCount % camera->GetMultiviewCameras()->GetCount();
						RN::Camera *multiviewCamera = camera->GetMultiviewCameras()->GetObjectAtIndex<RN::Camera>(multiviewIndex);
						matViewProj = multiviewCamera->GetProjectionMatrix() * multiviewCamera->GetViewMatrix();
					}

					{
						RN_PROFILE_SCOPE_N("Render Occluder Depth");

						//Render occluders to depth buffer first (first test if the bounding box is visible at all)
						for(SceneNode *node : occluders)
						{
                            bool testResult = _occlusionCuller->TestBoundingBox(matViewProj, node->GetBoundingBox(), screenPixelSize);
							SceneQuadtreeInfo *sceneInfo = static_cast<SceneQuadtreeInfo *>(node->GetSceneInfo());
							sceneInfo->isActiveOccluder = true;
							if(!testResult && sceneInfo->occludedFrameCounter < 1000)
							{
								sceneInfo->occludedFrameCounter += 1;
							}
							if(testResult || sceneInfo->occludedFrameCounter < OCCLUSION_FRAMECOUNT)
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
									Matrix matModelViewProj = matViewProj * node->GetWorldTransform();

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

							if(node->GetFlags() & SceneNode::Flags::Occluder)
							{
								SceneQuadtreeInfo *sceneInfo = static_cast<SceneQuadtreeInfo *>(node->GetSceneInfo());
								if(sceneInfo->isActiveOccluder && sceneInfo->occludedFrameCounter < OCCLUSION_FRAMECOUNT)
								{
									sceneNodesToRender.push_back(node);
									continue;
								}
							}

                            bool testResult = _occlusionCuller->TestBoundingBox(matViewProj, node->GetBoundingBox(), screenPixelSize);
							SceneQuadtreeInfo *sceneInfo = static_cast<SceneQuadtreeInfo *>(node->GetSceneInfo());
							if(!testResult && sceneInfo->occludedFrameCounter < 1000)
							{
								sceneInfo->occludedFrameCounter += 1;
							}
							if(testResult || sceneInfo->occludedFrameCounter < OCCLUSION_FRAMECOUNT)
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
				else*/
/*				if(camera->GetFlags() & Camera::Flags::UseUIFastPath)
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
				else*/
				{
					RN_PROFILE_SCOPE_N("Collect Entities");
					if(camera->GetRenderNodes())
					{
						//In this case the camera was given a list of nodes to render, don't visibility check them and ignore the actual scene
						camera->GetRenderNodes()->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {
							/*if(node->GetRenderPriority() == SceneNode::RenderTransparent)
							{
								if(firstTransparentIndex == 0)
								{
									firstTransparentIndex = sceneNodesToRender.size();
								}
								lastTransparentIndex = sceneNodesToRender.size();
							}*/
							sceneNodesToRender.push_back(node);
						});
					}
					else
					{
						//Traverse the tree to collect all visible nodes
						TraverseTree(camera, sceneNodesToRender);
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

			/*	if(camera->GetFlags() & Camera::Flags::SortTransparentBackToFront && firstTransparentIndex < lastTransparentIndex)
				{
					RN_PROFILE_SCOPE_N("Sort Transparent");
					const RN::Vector3 cameraWorldPosition = camera->GetWorldPosition();
					std::sort(sceneNodesToRender.begin() + firstTransparentIndex, sceneNodesToRender.begin() + lastTransparentIndex + 1, [cameraWorldPosition](SceneNode *a, SceneNode *b) {
						return a->GetWorldPosition().GetSquaredDistance(cameraWorldPosition) > b->GetWorldPosition().GetSquaredDistance(cameraWorldPosition);
					});
				}*/

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

	SceneQuadtree::TreeBounds SceneQuadtree::GetSceneNodeTreeBounds(const SceneNode *node) const
	{
		const AABB worldBounds = node->GetBoundingBox();
		const Vector3 localPosition(worldBounds.position - _treeOrigin);
		return TreeBounds(localPosition + worldBounds.minExtend, localPosition + worldBounds.maxExtend);
	}

	void SceneQuadtree::TraverseTree(RN::Camera *camera, std::vector<SceneNode *> &sceneNodesToRender)
	{
		RN_PROFILE_SCOPE();
		
		RN::Camera *lodCamera = camera->GetLODCamera();

		const Vector3 cameraPosition = lodCamera->GetRenderPosition();
		const float fovFactor = lodCamera->GetProjectionMatrix().m[0];
		const Vector3 treeLODOffset(_treeOrigin - lodCamera->GetRenderOrigin());
		
		std::vector<uint32> stack;
		stack.reserve(128);
		stack.push_back(0);

		int counter = 0;
		while(!stack.empty())
		{
			uint32 i = stack.back();
			stack.pop_back();
			const TreeNode& node = _treeNodes[i];
			AABB bounds(node.bounds.min, node.bounds.max);
			bounds.position = _treeOrigin;

			if(node.numberOfObjects == 0 || !lodCamera->InFrustum(bounds)) continue;
			
			counter += 1;

			//Visit all objects that are inside this node
			for(SceneNode *object : node.objects)
			{
				if((object->GetRenderGroup() & camera->GetRenderGroup()) == 0)
					continue;

				if(object->HasFlags(SceneNode::Flags::Hidden))
					continue;

				if(object->GetMaximumRenderDistance() > 0.0f && !object->HasFlags(SceneNode::Flags::NoCulling) && !camera->InFrustum(object->GetBoundingSphere(), object->GetMaximumRenderDistance()))
					continue;

				sceneNodesToRender.push_back(object);
			}

			//Node center and half extents relative to the LOD camera render origin
			const Vector3 center = (node.bounds.min + node.bounds.max) * 0.5f + treeLODOffset;
			const Vector3 half = (node.bounds.max - node.bounds.min) * 0.5f;

			//Depth = view-space z (along camera forward)
			const Vector3 v = center - cameraPosition;
			const float zView = v.GetLength();// std::max(v.GetDotProduct(cameraForward), 0.001f);

			//Horizontal radius
			const float sX = std::sqrt(half.x * half.x + half.z * half.z);
			
			//Projected radius in NDC (half-range is 1.0)
			float ndcRadiusX = (sX / zView) * fovFactor;

			//Push children
			if(node.firstChild != UINT32_MAX && ndcRadiusX > 0.2f)
			{
				for(int childIndex = 0; childIndex < 4; ++childIndex)
				{
					stack.push_back(node.firstChild + childIndex);
				}
			}
		}
		
		if(!(camera->GetFlags() & Camera::Flags::Orthogonal))
		{
			RNDebug("Visited " << counter << " nodes, clip far: " << camera->GetClipFar() << ", cameraPos: " << cameraPosition << ", camerRot: " << camera->GetWorldEulerAngle() << ", objects to render: " << sceneNodesToRender.size());
		}
	}

	uint32 SceneQuadtree::FindTreeNode(const TreeBounds &box, bool isInserting, uint8 maxDepth)
	{
		uint32 nodeIndex = 0; // root
		while(maxDepth > 0)
		{
			TreeNode &n = _treeNodes[nodeIndex];
			if(isInserting)
			{
				//Adjust the bounds to fit the new object vertically
				if(n.numberOfObjects == 0)
				{
					n.bounds.min.y = box.min.y;
					n.bounds.max.y = box.max.y;
				}
				else
				{
					n.bounds.min.y = std::min(n.bounds.min.y, box.min.y);
					n.bounds.max.y = std::max(n.bounds.max.y, box.max.y);
				}

				//Increase number of objects as moving down the tree when inserting something
				n.numberOfObjects += 1;
			}

			if(n.firstChild == UINT32_MAX)
			{
				return nodeIndex; // leaf
			}

			//Early-out if the object it relatively large compared to the node
			const Vector3 half = (box.max - box.min) * 0.5f;
			const float rObj = std::sqrt(half.x*half.x + half.y*half.y + half.z*half.z);
			const float nodeSizeX = n.bounds.max.x - n.bounds.min.x;
			const float nodeSizeZ = n.bounds.max.z - n.bounds.min.z;
			const float rNode = 0.5f * std::sqrt(nodeSizeX*nodeSizeX + nodeSizeZ*nodeSizeZ);
			if(rObj >= 0.4f * rNode) return nodeIndex;

			bool found = false;
			for(int i = 0; i < 4; ++i)
			{
				uint32 childIndex = n.firstChild + i;
				if(_treeNodes[childIndex].Contains(box))
				{
					nodeIndex = childIndex;
					found = true;
					break;
				}
			}

			if(!found)
			{
				return nodeIndex; //No more child node found that the objects fully fits into
			}

			maxDepth -= 1;
		}

		return nodeIndex;
	}

	void SceneQuadtree::AddRenderNode(SceneNode *node)
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
		
		SceneQuadtreeInfo *sceneInfo = static_cast<SceneQuadtreeInfo*>(node->GetSceneInfo());
		TreeBounds box = GetSceneNodeTreeBounds(node);
		uint32 n = FindTreeNode(box, true, sceneInfo->maxDepth);
		_treeNodes[n].objects.push_back(node);
		sceneInfo->quadtreeNodeIndex = n;

		Unlock();
	}

	void SceneQuadtree::RemoveRenderNode(SceneNode *node)
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

		SceneQuadtreeInfo *sceneInfo = static_cast<SceneQuadtreeInfo*>(node->GetSceneInfo());
		uint32 nodeIndex = sceneInfo->quadtreeNodeIndex;
		if(nodeIndex == UINT32_MAX) return;

		auto& list = _treeNodes[nodeIndex].objects;
		auto it = std::find(list.begin(), list.end(), node);
		if(it != list.end())
		{
			*it = list.back();
			list.pop_back();
		}

		sceneInfo->quadtreeNodeIndex = UINT32_MAX;
	}

	void SceneQuadtree::RelocateNodeIfNeeded(SceneNode *node)
	{
		node->GetChildren()->Enumerate<SceneNode>([&](SceneNode *child, size_t index, bool &stop) {
			RelocateNodeIfNeeded(child);
		});
		
		SceneQuadtreeInfo *sceneInfo = static_cast<SceneQuadtreeInfo*>(node->GetSceneInfo());
		if(!sceneInfo) return;
		uint32 nodeIndex = sceneInfo->quadtreeNodeIndex;
		if(nodeIndex == UINT32_MAX) return; // Not a render node in the quadtree

		TreeBounds box = GetSceneNodeTreeBounds(node);
		// Fast path: still contained in current leaf in XZ
		if(_treeNodes[nodeIndex].Contains(box)) return;

		// Reinsert into the correct leaf
		Lock();
		// Refresh current index in case another thread already moved it
		uint32 currentIndex = sceneInfo->quadtreeNodeIndex;
		if(currentIndex != UINT32_MAX)
		{
			auto &list = _treeNodes[currentIndex].objects;
			auto it = std::find(list.begin(), list.end(), node);
			if(it != list.end())
			{
				*it = list.back();
				list.pop_back();
			}
		}

		uint32 newIndex = FindTreeNode(box, false, sceneInfo->maxDepth);
		_treeNodes[newIndex].objects.push_back(node);
		sceneInfo->quadtreeNodeIndex = newIndex;
		Unlock();
	}

	void SceneQuadtree::AddNode(SceneNode *node, uint8 maxDepth)
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
		SceneQuadtreeInfo *sceneInfo = new SceneQuadtreeInfo(this);
		sceneInfo->maxDepth = maxDepth;
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

	void SceneQuadtree::AddNode(SceneNode *node)
	{
		AddNode(node, UINT8_MAX);
	}

	void SceneQuadtree::RemoveNode(SceneNode *node)
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
