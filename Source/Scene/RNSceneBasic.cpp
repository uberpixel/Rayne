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
#include "../Scene/RNEntity.h"
#include "../Rendering/RNModel.h"
#include "../Rendering/RNMesh.h"
#include "../Math/RNRandom.h"

#define kRNSceneUpdateBatchSize 8192 //1024
#define kRNSceneRenderBatchSize 32

namespace RN
{
	RNDefineMeta(SceneBasic, Scene)
	RNDefineMeta(SceneBasicInfo, SceneInfo)

	RN_INLINE float edgeFunction(const Vector2 a, const Vector2 b, const Vector2 c)
	{
		return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
	}

	SceneBasicInfo::SceneBasicInfo(Scene *scene) : SceneInfo(scene), occludedFrameCounter(0)
	{
		
	}

	SceneBasic::SceneBasic() : _nodesToRemove(new Array()), _occlusionDepthBufferWidth(40), _occlusionDepthBufferHeight(40), _currentFrameCount(0)
	{
		_occlusionDepthBuffer = new float[_occlusionDepthBufferWidth * _occlusionDepthBufferHeight];
	}
	
	SceneBasic::~SceneBasic()
	{
		_nodesToRemove->Release();
		delete _occlusionDepthBuffer;
	}

	void SceneBasic::Update(float delta)
	{
		WillUpdate(delta);

		WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default);

		for(size_t i = 0; i < 4; i ++)
		{
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
//				group->Perform(queue, [&, member, first] {

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

	void SceneBasic::FlushDeletionQueue()
	{
		bool didUpdateCameras = false;
		_nodesToRemove->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {

			if(node->IsKindOfClass(Camera::GetMetaClass()))
			{
				Camera *camera = static_cast<Camera *>(node);
				_cameras.Erase(camera->_cameraSceneEntry);
				didUpdateCameras = true;
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
			node->_scheduledForRemovalFromScene = false; //Make sure it can be added and removed again, if the object doesn't actually get deleted here!
			node->Autorelease(); //Autorelease here causes the constructors to be called later (otherwise RemoveAllObjects below would do it), this will cause objects removed from the scene in the destructor to be removed in the next frame, working around some issue I've been having otherwise
			//TODO: Somehow make objects removed in the destructor also be deleted here
		});

		_nodesToRemove->RemoveAllObjects();
	}

	void SceneBasic::RasterizeClipSpaceTriangle(Vector4 A, Vector4 B, Vector4 C)
	{
		A /= A.w;
		A.x = A.x * 0.5f + 0.5f;
		A.x *= _occlusionDepthBufferWidth;
		A.y = A.y * 0.5f + 0.5f;
		A.y *= _occlusionDepthBufferHeight;
		
		B /= B.w;
		B.x = B.x * 0.5f + 0.5f;
		B.x *= _occlusionDepthBufferWidth;
		B.y = B.y * 0.5f + 0.5f;
		B.y *= _occlusionDepthBufferHeight;
		
		C /= C.w;
		C.x = C.x * 0.5f + 0.5f;
		C.x *= _occlusionDepthBufferWidth;
		C.y = C.y * 0.5f + 0.5f;
		C.y *= _occlusionDepthBufferHeight;
		
		float area = edgeFunction(Vector2(A), Vector2(B), Vector2(C));
		if(area < 0) return; //Triangle is facing away, can just skip completely at this point
		
		uint16 minX = std::min(std::max(std::min(A.x, std::min(B.x, C.x)), 0.0f), static_cast<float>(_occlusionDepthBufferWidth-1));
		uint16 minY = std::min(std::max(std::min(A.y, std::min(B.y, C.y)), 0.0f), static_cast<float>(_occlusionDepthBufferHeight-1));
		
		uint16 maxX = std::min(std::max(std::max(A.x, std::max(B.x, C.x)), 0.0f), static_cast<float>(_occlusionDepthBufferWidth-1));
		uint16 maxY = std::min(std::max(std::max(A.y, std::max(B.y, C.y)), 0.0f), static_cast<float>(_occlusionDepthBufferHeight-1));
		
		for(uint16 y = minY; y <= maxY; y++)
		{
			for(uint16 x = minX; x <= maxX; x++)
			{
				Vector2 point(x, y);
				
				float w0 = edgeFunction(Vector2(B), Vector2(C), point);
				float w1 = edgeFunction(Vector2(C), Vector2(A), point);
				float w2 = edgeFunction(Vector2(A), Vector2(B), point);
				if(w0 >= 0 && w1 >= 0 && w2 >= 0)
				{
					w0 /= area;
					w1 /= area;
					w2 /= area;
					float depth = w0 * A.z + w1 * B.z + w2 * C.z - 0.000001f; //Add a bit of an offset to prevent precision issues
					
					if(depth <= 1.0f)
					{
						//depth *= 10000.0f;
						if(depth > _occlusionDepthBuffer[_occlusionDepthBufferWidth * (_occlusionDepthBufferHeight - y - 1) + x])
						{
							_occlusionDepthBuffer[_occlusionDepthBufferWidth * (_occlusionDepthBufferHeight - y - 1) + x] = depth;
						}
					}
				}
			}
		}
	}

	void SceneBasic::RasterizeMesh(const Matrix &matModelViewProj, Mesh *mesh)
	{
		RN::Mesh::Chunk chunk = mesh->GetTrianglesChunk();
		Mesh::ElementIterator<Vector3> iterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);
		size_t triangleCount = mesh->GetIndicesCount() / 3;
		for(size_t i = 0; i < triangleCount; i++)
		{
			const Vector3 &posA = *iterator++;
			const Vector3 &posB = *iterator++;
			const Vector3 &posC = *iterator;
			if(i < triangleCount-1)
			{
				iterator++;
			}
			
			Vector4 A = matModelViewProj * Vector4(posB, 1.0f);
			Vector4 B = matModelViewProj * Vector4(posA, 1.0f);
			Vector4 C = matModelViewProj * Vector4(posC, 1.0f);
			
			//Skip triangle if all vertices are on the same side of the near or far clip planes
			if(A.z < -A.w && B.z < -B.w && C.z < -C.w) continue;
			if(A.z > A.w && B.z > B.w && C.z > C.w) continue;
			
			//Handle clipping of triangles that intersect the near plane, otherwise dividing by w will cause all kind of issues
			if(A.z > A.w && B.z > B.w)
			{
				//Both A and B are clipped, move them both onto the clipping plane towards C
				float dA = (A.z - (A.w));
				float dB = (B.z - (B.w));
				float dC = (C.z - (C.w));
				float tA = dA/(dA-dC);
				float tB = dB/(dB-dC);
				
				A = A + (C-A) * tA;
				B = B + (C-B) * tB;
			}
			else if(A.z > A.w && C.z > C.w)
			{
				//Both A and C are clipped, move them both onto the clipping plane towards B
				float dA = (A.z - (A.w));
				float dB = (B.z - (B.w));
				float dC = (C.z - (C.w));
				float tA = dA/(dA-dB);
				float tC = dC/(dC-dB);
				
				A = A + (B-A) * tA;
				C = C + (B-C) * tC;
			}
			else if(B.z > B.w && C.z > C.w)
			{
				//Both B and C are clipped, move them both onto the clipping plane towards A
				float dA = (A.z - (A.w));
				float dB = (B.z - (B.w));
				float dC = (C.z - (C.w));
				float tB = dB/(dB-dA);
				float tC = dC/(dC-dA);
				
				B = B + (A-B) * tB;
				C = C + (A-C) * tC;
			}
			else if(A.z > A.w)
			{
				//Only A is clipped, needs to create two new points on the clipplane to replace it with
				float dA = (A.z - (A.w));
				float dB = (B.z - (B.w));
				float dC = (C.z - (C.w));
				float tB = dA/(dA-dB);
				float tC = dA/(dA-dC);
				
				Vector4 AB = A + (B-A) * tB;
				Vector4 AC = A + (C-A) * tC;
				A = AB;
				
				RasterizeClipSpaceTriangle(C, AC, AB);
			}
			else if(B.z > B.w)
			{
				//Only B is clipped, needs to create two new points on the clipplane to replace it with
				float dA = (A.z - (A.w));
				float dB = (B.z - (B.w));
				float dC = (C.z - (C.w));
				float tA = dB/(dB-dA);
				float tC = dB/(dB-dC);
				
				Vector4 BA = B + (A-B) * tA;
				Vector4 BC = B + (C-B) * tC;
				B = BA;
				
				RasterizeClipSpaceTriangle(C, BA, BC);
			}
			else if(C.z > C.w)
			{
				//Only B is clipped, needs to create two new points on the clipplane to replace it with
				float dA = (A.z - (A.w));
				float dB = (B.z - (B.w));
				float dC = (C.z - (C.w));
				float tA = dC/(dC-dA);
				float tB = dC/(dC-dB);
				
				Vector4 CA = C + (A-C) * tA;
				Vector4 CB = C + (B-C) * tB;
				C = CA;
				
				RasterizeClipSpaceTriangle(B, CB, CA);
			}
			
			RasterizeClipSpaceTriangle(A, B, C);
		}
	}

	bool SceneBasic::TestBoundingBox(const Matrix &matViewProj, const AABB &aabb, const Vector2 &screenPixelSize)
	{
		//Get bounding box corners
		Vector4 boxCorners[8];

		const Vector4 position(aabb.position, 1.0f);
		boxCorners[0] = position + Vector4(aabb.maxExtend.x, aabb.maxExtend.y, aabb.maxExtend.z, 0.0f);
		boxCorners[1] = position + Vector4(aabb.maxExtend.x, aabb.maxExtend.y, aabb.minExtend.z, 0.0f);
		boxCorners[2] = position + Vector4(aabb.maxExtend.x, aabb.minExtend.y, aabb.minExtend.z, 0.0f);
		boxCorners[3] = position + Vector4(aabb.maxExtend.x, aabb.minExtend.y, aabb.maxExtend.z, 0.0f);
		boxCorners[4] = position + Vector4(aabb.minExtend.x, aabb.maxExtend.y, aabb.maxExtend.z, 0.0f);
		boxCorners[5] = position + Vector4(aabb.minExtend.x, aabb.maxExtend.y, aabb.minExtend.z, 0.0f);
		boxCorners[6] = position + Vector4(aabb.minExtend.x, aabb.minExtend.y, aabb.minExtend.z, 0.0f);
		boxCorners[7] = position + Vector4(aabb.minExtend.x, aabb.minExtend.y, aabb.maxExtend.z, 0.0f);
		
		Vector3 maxCorners;
		Vector3 minCorners;
		
		//Project corners to camera 2D plane
		for(int i = 0; i < 8; i++)
		{
			boxCorners[i] = matViewProj * boxCorners[i];
			
			if(boxCorners[i].z > boxCorners[i].w)
			{
				//Bounding box intersects near clipping plane, assume as visible
				return true;
			}
			
			boxCorners[i] /= boxCorners[i].w;
			boxCorners[i].x = boxCorners[i].x * 0.5f + 0.5f;
			boxCorners[i].y = boxCorners[i].y * 0.5f + 0.5f;
			
			if(i == 0)
			{
				maxCorners = minCorners = Vector3(boxCorners[i]);
			}
			else
			{
				maxCorners.x = std::max(maxCorners.x, boxCorners[i].x);
				maxCorners.y = std::max(maxCorners.y, boxCorners[i].y);
				maxCorners.z = std::max(maxCorners.z, boxCorners[i].z);
				
				minCorners.x = std::min(minCorners.x, boxCorners[i].x);
				minCorners.y = std::min(minCorners.y, boxCorners[i].y);
				//minCorners.z = std::min(minCorners.z, boxCorners[i].z); //unused
			}
			
			//depth is 1 closest to the camera and goes down to 0 the further away it is
		}
		
		//Fail depth test for objects that are smaller than a single pixel
		if((maxCorners.x - minCorners.x) < screenPixelSize.x && (maxCorners.y - minCorners.y) < screenPixelSize.y)
		{
			//TODO: Above should use "or" but somehow tends to cull things that aren't really that small...
			//RNDebug("failed size test: " << (maxCorners.x - minCorners.x) << " < " << screenPixelSize.x << ", " << (maxCorners.y - minCorners.y) << " < " << screenPixelSize.y);
			return false;
		}
		
		minCorners.x *= _occlusionDepthBufferWidth;
		minCorners.y *= _occlusionDepthBufferHeight;
		
		maxCorners.x *= _occlusionDepthBufferWidth;
		maxCorners.y *= _occlusionDepthBufferHeight;
		
		uint16 minX = std::max(std::min(std::floor(minCorners.x) - 1.0f, static_cast<float>(_occlusionDepthBufferWidth - 1)), 0.0f);
		uint16 maxX = std::max(std::min(std::ceil(maxCorners.x) + 1.0f, static_cast<float>(_occlusionDepthBufferWidth - 1)), 0.0f);
		
		uint16 minY = std::max(std::min(std::floor(minCorners.y) - 1.0f, static_cast<float>(_occlusionDepthBufferHeight - 1)), 0.0f);
		uint16 maxY = std::max(std::min(std::ceil(maxCorners.y) + 1.0f, static_cast<float>(_occlusionDepthBufferHeight - 1)), 0.0f);

		//maxCorners.z *= 10000.0f;

		for(uint16 y = minY; y <= maxY; y++)
		{
			for(uint16 x = minX; x <= maxX; x++)
			{
				if(maxCorners.z > _occlusionDepthBuffer[_occlusionDepthBufferWidth * (_occlusionDepthBufferHeight - y - 1) + x])
				{
					return true;
				}
			}
		}
		
		return false;
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

				std::vector<SceneNode *> occluders;
				std::vector<SceneNode *> sceneNodesToRender;
				
				//Collect all occluders
				IntrusiveList<SceneNode>::Member *nodeMember = _renderNodes.GetHead();
				while(nodeMember)
				{
					SceneNode *node = nodeMember->Get();
					if(node->HasFlags(SceneNode::Flags::Occluder) && node->CanRender(renderer, camera))
					{
						occluders.push_back(node);
					}

					nodeMember = nodeMember->GetNext();
				}
				
				//Do occlusion culling if there are 1 or more occluders!
				nodeMember = _renderNodes.GetHead();
				if(occluders.size() > 0)
				{
					std::vector<SceneNode *> visibleOccluders;
					
					//Sort occluders by approximated size on the screen
					std::sort(occluders.begin(), occluders.end(), [camera](
							SceneNode *a, SceneNode *b) {
						float distanceA = std::max(a->GetWorldPosition().GetDistance(camera->GetWorldPosition()), 1.0f);
						float distanceB = std::max(b->GetWorldPosition().GetDistance(camera->GetWorldPosition()), 1.0f);
						
						return a->GetBoundingSphere().radius / distanceA > b->GetBoundingSphere().radius / distanceB;
					});
					
					occluders.resize(std::min(static_cast<size_t>(30), occluders.size())); //Only keep the biggest 30 occluders in the list
					
					//Sort remaining occluders front to back
					std::sort(occluders.begin(), occluders.end(), [camera](
							SceneNode *a, SceneNode *b) {
						return a->GetWorldPosition().GetSquaredDistance(camera->GetWorldPosition()) < b->GetWorldPosition().GetSquaredDistance(camera->GetWorldPosition());
					});
					
					//Clear occlusion depth map
					std::fill(_occlusionDepthBuffer, _occlusionDepthBuffer + _occlusionDepthBufferWidth * _occlusionDepthBufferHeight, 0.0f);
					
					Vector2 screenPixelSize = Vector2(1.0f/camera->GetRenderPass()->GetFrame().width, 1.0f/camera->GetRenderPass()->GetFrame().height);
					
					Vector3 randomCameraOffset = RandomNumberGenerator::GetSharedGenerator()->GetRandomVector3Range(RN::Vector3(-0.15f, -0.15f, 0.0f), RN::Vector3(0.15f, 0.15f, 0.0f));
					Matrix matViewProj = camera->GetProjectionMatrix() * Matrix::WithTranslation(randomCameraOffset) * camera->GetViewMatrix();
					if(camera->GetIsMultiviewCamera())
					{
						size_t multiviewIndex = _currentFrameCount % camera->GetMultiviewCameras()->GetCount();
						RN::Camera *multiviewCamera = camera->GetMultiviewCameras()->GetObjectAtIndex<RN::Camera>(multiviewIndex);
						matViewProj = multiviewCamera->GetProjectionMatrix() * multiviewCamera->GetViewMatrix();
					}
					
					//Render occluders to depth buffer first (first test if the bounding box is visible at all)
					for(SceneNode *node : occluders)
					{
						bool testResult = TestBoundingBox(matViewProj, node->GetBoundingBox(), screenPixelSize);
						SceneBasicInfo *sceneInfo = node->GetSceneInfo()->Downcast<SceneBasicInfo>();
						if(!testResult && sceneInfo->occludedFrameCounter < 1000)
						{
							sceneInfo->occludedFrameCounter += 1;
						}
						if(testResult || sceneInfo->occludedFrameCounter < 50)
						{
							if(testResult)
							{
								sceneInfo->occludedFrameCounter = 0;
							}
							
							visibleOccluders.push_back(node);
						}
						
						if(testResult)
						{
							//TODO: Deal with models that have multiple meshes, also what lod stage should be used if there are multiple?
							RN::Entity *entity = node->Downcast<Entity>();
							if(entity)
							{
								Model *model = entity->GetModel();
								RN::Mesh *mesh = model->GetLODStage(0)->GetMeshAtIndex(0);
								Matrix matModelViewProj = matViewProj * node->GetWorldTransform();
								RasterizeMesh(matModelViewProj, mesh);
							}
						}
					}
					
					//Test all visible objects against depth buffer
					while(nodeMember)
					{
						SceneNode *node = nodeMember->Get();
						nodeMember = nodeMember->GetNext();
						if(!node->CanRender(renderer, camera)) continue;
						if(node->GetRenderPriority() >= SceneNode::RenderSky)
						{
							sceneNodesToRender.push_back(node);
							continue;
						}
						
						//TODO: Find a better way to check if an occluder is visible and should be added to the render queue
						if(node->GetFlags() & SceneNode::Flags::Occluder && std::find(visibleOccluders.begin(), visibleOccluders.end(), node) != visibleOccluders.end())
						{
							sceneNodesToRender.push_back(node);
							continue;
						}
						
						bool testResult = TestBoundingBox(matViewProj, node->GetBoundingBox(), screenPixelSize);
						SceneBasicInfo *sceneInfo = node->GetSceneInfo()->Downcast<SceneBasicInfo>();
						if(!testResult && sceneInfo->occludedFrameCounter < 1000)
						{
							sceneInfo->occludedFrameCounter += 1;
						}
						if(testResult || sceneInfo->occludedFrameCounter < 50)
						{
							if(testResult)
							{
								sceneInfo->occludedFrameCounter = 0;
							}
							
							sceneNodesToRender.push_back(node);
						}
					}
				}
				else
				{
					while(nodeMember)
					{
						SceneNode *node = nodeMember->Get();
						if(node->CanRender(renderer, camera))
						{
							sceneNodesToRender.push_back(node);
						}

						nodeMember = nodeMember->GetNext();
					}
				}

				if(camera->GetFlags() & Camera::Flags::SortFrontToBack)
				{
					std::sort(sceneNodesToRender.begin(), sceneNodesToRender.end(), [camera](
							SceneNode *a, SceneNode *b) {
						if(a->GetRenderPriority() == b->GetRenderPriority() && b->GetRenderPriority() < SceneNode::RenderSky)
						{
							return a->GetWorldPosition().GetSquaredDistance(camera->GetWorldPosition()) < b->GetWorldPosition().GetSquaredDistance(camera->GetWorldPosition());
						}
						return a->GetRenderPriority() < b->GetRenderPriority();
					});
				}

				//RNInfo("Number of objects: " << sceneNodesToRender.size());

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
					for(SceneNode *node : sceneNodesToRender)
					{
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
	}

	void SceneBasic::RemoveRenderNode(SceneNode *node)
	{
		_renderNodes.Erase(node->_sceneRenderEntry);
	}

	void SceneBasic::AddNode(SceneNode *node)
	{
		//Remove from deletion list if scheduled for deletion if the scene didn't change.
		if(node->GetSceneInfo() && node->GetSceneInfo()->GetScene() == this && node->_scheduledForRemovalFromScene)
		{
			_nodesToRemove->Lock();
			_nodesToRemove->RemoveObject(node);
			_nodesToRemove->Unlock();
			node->_scheduledForRemovalFromScene = false;
			return;
		}
		
		RN_ASSERT(node->GetSceneInfo() == nullptr, "AddNode() must be called on a Node not owned by a scene");
		
		node->Retain();
		SceneBasicInfo *sceneInfo = new SceneBasicInfo(this);
		node->UpdateSceneInfo(sceneInfo->Autorelease());
    
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
		
		//Lock to prevent race condition of multiple threads adding nodes at the same time
		Lock();
		//PushFront to prevent race condition with scene iterating over the nodes.
		_updateNodes[static_cast<size_t>(node->GetUpdatePriority())].PushFront(node->_sceneUpdateEntry);
		Unlock();
	}
	
	void SceneBasic::RemoveNode(SceneNode *node)
	{
		RN_ASSERT(node->GetSceneInfo() && node->GetSceneInfo()->GetScene() == this && node->_scheduledForRemovalFromScene == false, "RemoveNode() must be called on a Node owned by the scene");
		
		_nodesToRemove->Lock();
		_nodesToRemove->AddObject(node);
		_nodesToRemove->Unlock();
		node->_scheduledForRemovalFromScene = true;
	}
}
