//
//  RNCamera.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCamera.h"
#include "../Rendering/RNRenderer.h"
#include "../Rendering/RNWindow.h"
#include "RNLight.h"
#include "RNLightManager.h"

namespace RN
{
	RNDefineMeta(Camera, SceneNode)

	Camera::Camera(RenderPass *renderPass) :
		_cameraSceneEntry(this), _flags(Camera::Flags::Defaults)
	{
		if(!renderPass)
		{
			_renderPass = new RenderPass();
		}
		else
		{
			_renderPass = renderPass->Retain();
		}
		_rootFramePass = nullptr;

		Initialize();
	}

	Camera::Camera(const Vector2 &size) :
		_cameraSceneEntry(this),
		_flags(Camera::Flags::Defaults)
	{
		//TODO: This implementaiton is probably not what a user would expect when creating a camera with a size tbh... It just renders it to the screens top left corner with the given size. Instead it should probably create a render target of that size and render into it instead.
		_renderPass = new RenderPass();
		_renderPass->SetFrame(RN::Rect(0, 0, size.x, size.y));
		_rootFramePass = nullptr;

		Initialize();
	}

	Camera::~Camera()
	{
		SafeRelease(_renderPass);
		SafeRelease(_rootFramePass);

		if(_multiviewCameras)
		{
			_multiviewCameras->Enumerate<Camera>([](Camera *camera, size_t index, bool &stop) {
				camera->_multiviewParentCamera = nullptr;
			});
		}
		SafeRelease(_multiviewCameras);
		SafeRelease(_renderNodes);

		SafeRelease(_lightManager);
	}


	void Camera::Initialize()
	{
		_fov = 70.0f;
		_aspect = 0.0f;

		_clipNear = 0.1f;
		_clipFar = 500.0f;
		_referenceFar = 0.0f;

		_orthoLeft = -100.0f;
		_orthoRight = 100.0f;
		_orthoBottom = -100.0f;
		_orthoTop = 100.0f;

		_fogNear = 100.0f;
		_fogFar = 500.0f;
		_ambient = Color::White();
		_customData = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

		_dirtyProjection = true;
		_dirtyPosition = true;
#if RN_ENABLE_LAZY_FRAME_ROOT_TRANSFORMS
		_preparedTransformVersion = 0;
#endif
		_dirtyFrustum = true;
		_hasCustomNearClipPlane = false;
		_hasExplicitProjectionMatrix = false;
		_customNearClipPlane = Plane();
		_renderOrigin = PositionType();
#if RN_ENABLE_UNIVERSE_SCALE && RN_ENABLE_LAZY_FRAME_ROOT_TRANSFORMS
		_cachedRenderOriginVersion = 0;
#endif

		_priority = 0;
		_lodCamera = nullptr;

		_multiviewCameras = nullptr;
		_multiviewParentCamera = nullptr;

		_frustumPlaneOffsets[0] = 0.0f;
		_frustumPlaneOffsets[1] = 0.0f;
		_frustumPlaneOffsets[2] = 0.0f;
		_frustumPlaneOffsets[3] = 0.0f;

		_firstNodeMember = nullptr;

		_renderNodes = nullptr;

		_lightManager = nullptr;
	}

	// Setter
	void Camera::SetRenderPass(RenderPass *renderPass)
	{
		RN_ASSERT(renderPass, "Camera render pass mustn't be NULL");
		RN_ASSERT(!renderPass->GetIsSubpass(), "Subpass render passes have to be inside a root renderpass that is not a sub pass");

		SafeRelease(_renderPass);
		_renderPass = renderPass->Retain();

		if(_rootFramePass == _renderPass)
		{
			SafeRelease(_rootFramePass);
			_rootFramePass = nullptr;
		}
	}

	void Camera::SetRootFramePass(FramePass *framePass)
	{
		if(!framePass || framePass == _renderPass)
		{
			SafeRelease(_rootFramePass);
			_rootFramePass = nullptr;
			return;
		}

		RenderPass *renderPass = framePass->Downcast<RenderPass>();
		RN_ASSERT(!renderPass || !renderPass->GetIsSubpass(), "Subpass render passes have to be inside a root renderpass that is not a sub pass");

		SafeRelease(_rootFramePass);
		_rootFramePass = framePass->Retain();
	}

	void Camera::SetFlags(Flags flags)
	{
		RN_DEBUG_ASSERT(!(flags & Flags::Orthogonal) || !isinf(_clipFar), "Orthogonal cameras do not support an infinite far clip plane");
		RN_ASSERT(!(flags & Flags::SortFrontToBack) || !(flags & Flags::SortInstancable), "SortFrontToBack and SortInstancable cannot be enabled together");
		const bool orthogonalChanged = static_cast<bool>(_flags & Flags::Orthogonal) != static_cast<bool>(flags & Flags::Orthogonal);
		const bool simpleCullingChanged = static_cast<bool>(_flags & Flags::UseSimpleCulling) != static_cast<bool>(flags & Flags::UseSimpleCulling);
		_flags = flags;
		if(orthogonalChanged) _dirtyProjection = true;
		if(simpleCullingChanged) _dirtyFrustum = true;
	}

	void Camera::SetLODCamera(Camera *camera)
	{
		_lodCamera = camera;
	}

	void Camera::SetPriority(int32 priority)
	{
		_priority = priority;
	}

	void Camera::SetFOV(float fov)
	{
		_fov = fov;
		_hasExplicitProjectionMatrix = false;
		_dirtyProjection = true;
	}
	void Camera::SetAspectRatio(float ratio)
	{
		_aspect = ratio;
		_hasExplicitProjectionMatrix = false;
		_dirtyProjection = true;
	}

	void Camera::SetClipNear(float near)
	{
		_clipNear = near;
		_hasExplicitProjectionMatrix = false;
		_dirtyProjection = true;
	}
	void Camera::SetClipFar(float far)
	{
		RN_DEBUG_ASSERT(!(_flags & Flags::Orthogonal) || !isinf(far), "Orthogonal cameras do not support an infinite far clip plane");
		_clipFar = far;
		_hasExplicitProjectionMatrix = false;
		_dirtyProjection = true;
	}

	void Camera::SetClipFarUnlimited()
	{
		SetClipFar(INFINITY);
	}

	void Camera::SetReferenceFar(float far)
	{
		RN_DEBUG_ASSERT(far == 0.0f || (far > 0.0f && !isinf(far)), "Camera reference far needs to be positive and finite, or 0 to use the automatic value");
		_referenceFar = far;
	}

	void Camera::SetFogColor0(Color color)
	{
		_fogColor0 = color;
	}
	void Camera::SetFogColor1(Color color)
	{
		_fogColor1 = color;
	}
	void Camera::SetFogNear(float near)
	{
		_fogNear = near;
	}
	void Camera::SetFogFar(float far)
	{
		_fogFar = far;
	}

	void Camera::SetAmbientColor(Color color)
	{
		_ambient = color;
	}

	void Camera::SetCustomData(const Vector4 &data)
	{
		_customData = data;
	}
	void Camera::SetCustomNearClipPlane(const Plane &clipPlane, bool enabled)
	{
		_customNearClipPlane = clipPlane;
		_hasCustomNearClipPlane = enabled;
		_dirtyProjection = true;
	}

	void Camera::SetOrthogonalFrustum(float top, float bottom, float left, float right)
	{
		RN_ASSERT((_flags & Flags::Orthogonal), "SetOrthogonalFrustum() called, but the camera is not an orthogonal camera");

		_orthoLeft = left;
		_orthoRight = right;
		_orthoTop = top;
		_orthoBottom = bottom;

		_hasExplicitProjectionMatrix = false;
		_dirtyProjection = true;
	}

	void Camera::SetFrustumPlaneOffset(float topOffset, float bottomOffset, float leftOffset, float rightOffset)
	{
		_frustumPlaneOffsets[0] = topOffset;
		_frustumPlaneOffsets[1] = bottomOffset;
		_frustumPlaneOffsets[2] = leftOffset;
		_frustumPlaneOffsets[3] = rightOffset;
		_dirtyFrustum = true;
	}

	void Camera::SetProjectionMatrix(const Matrix &projectionMatrix)
	{
		_explicitProjectionMatrix = projectionMatrix;
		_hasExplicitProjectionMatrix = true;
		_dirtyProjection = true;
		PostUpdate();
	}

	void Camera::DidUpdate(ChangeSet changeSet)
	{
		SceneNode::DidUpdate(changeSet);

		if(changeSet & (ChangeSet::Position | ChangeSet::Parent))
		{
			_dirtyPosition = true;
		}
	}

	Matrix Camera::MakeShadowSplit(Camera *camera, const Quaternion &shadowRotation, float cameraDistanceToCenter, float near, float far)
	{
		Rect frame = _renderPass->GetFrame();
		camera->PostUpdate();
		const PositionType renderOrigin = camera->GetRenderOrigin();

		//Get camera frustum extends to be covered by the split
		//far plane is at z=0, near plane z=1 for reverse-z!
		Vector3 nearcenter = camera->ToRender(Vector3(0.0f, 0.0f, near));
		Vector3 farcorner1 = camera->ToRender(Vector3(1.0f, 1.0f, far));
		Vector3 farcorner2 = camera->ToRender(Vector3(-1.0f, -1.0f, far));
		Vector3 farcenter = (farcorner1 + farcorner2) * 0.5f;
		Vector3 center = (nearcenter + farcenter) * 0.5f;

		//Calculate the size of a pixel in world units
		float dist = center.GetDistance(farcorner1);
		Vector3 pixelsize = Vector3(Vector2(dist * 2.0f), 1.0f) / Vector3(frame.width, frame.height, 1.0f);

		//Place the light camera above the splits center
		Vector3 pos = center - shadowRotation.GetRotatedVector(Vector3(0.0f, 0.0f, -1.0f)) * cameraDistanceToCenter;

		//Transform the position to light space
		Matrix rot = shadowRotation.GetRotationMatrix();
		pos = rot.GetInverse() * pos;

		//Snap to the pixel grid
		pos /= pixelsize;
		pos.x = roundf(pos.x);
		pos.y = roundf(pos.y);
		pos.z = roundf(pos.z);
		pos *= pixelsize;

		//Transform back and place the camera there
		pos = rot * pos;
		SetWorldPosition(renderOrigin + pos);

		//Set the light camera frustum
		_orthoLeft = -dist;
		_orthoRight = dist;
		_orthoBottom = -dist;
		_orthoTop = dist;

		//Update the projection matrix
		_hasExplicitProjectionMatrix = false;
		_dirtyProjection = true;
		UpdateProjection(); //Because the target is always a valid framebuffer, we don't need to pass the renderer as parameter here

		return _projectionMatrix * GetInverseWorldTransform(renderOrigin);
	}

	// Helper
	void Camera::Update(float delta)
	{
		SceneNode::Update(delta);
		_dirtyPosition = true;
		if(_hasCustomNearClipPlane) _dirtyProjection = true;
	}

	void Camera::PostUpdate()
	{
		PrepareRender(ResolveRenderOrigin());
	}

	void Camera::PrepareRender(const PositionType &renderOrigin)
	{
#if RN_ENABLE_LAZY_FRAME_ROOT_TRANSFORMS
		const uint64 transformVersion = GetTransformVersion();
		if(_preparedTransformVersion != transformVersion)
		{
			_preparedTransformVersion = transformVersion;
			_dirtyPosition = true;
		}
#endif
		if(_renderOrigin != renderOrigin)
		{
			_renderOrigin = renderOrigin;
			_dirtyPosition = true;
		}

		if(_dirtyPosition)
		{
			_dirtyPosition = false;
			_dirtyFrustum = true;
			if(_hasCustomNearClipPlane) _dirtyProjection = true;
			const Vector3 renderPosition = GetRenderPosition();

			_inverseViewMatrix = Matrix::WithTranslation(renderPosition);
			_inverseViewMatrix.Rotate(GetWorldRotation());
			_inverseViewMatrix.Scale(GetScale());

			_viewMatrix = _inverseViewMatrix.GetInverse();
		}

		UpdateProjection();
		UpdateFrustum();

		if(_multiviewCameras)
		{
			_multiviewCameras->Enumerate<Camera>([&](Camera *camera, size_t index, bool &stop) {
				camera->PrepareRender(renderOrigin);
			});
		}
	}

	PositionType Camera::ResolveRenderOrigin() const
	{
#if RN_ENABLE_UNIVERSE_SCALE
		const Camera *originCamera = _multiviewParentCamera ? _multiviewParentCamera : this;
#if RN_ENABLE_LAZY_FRAME_ROOT_TRANSFORMS
		const uint64 transformVersion = originCamera->GetTransformVersion();
		if(originCamera->_cachedRenderOriginVersion != transformVersion)
		{
			originCamera->_cachedRenderOrigin = originCamera->GetWorldPosition();
			originCamera->_cachedRenderOriginVersion = transformVersion;
		}
		return originCamera->_cachedRenderOrigin;
#else
		return originCamera->GetWorldPosition();
#endif
#else
		return PositionType();
#endif
	}

	void Camera::UpdateProjection()
	{
		if(!_dirtyProjection)
			return;

		if(_hasExplicitProjectionMatrix)
		{
			_projectionMatrix = _explicitProjectionMatrix;
		}
		else if(_flags & Flags::Orthogonal)
		{
			RN_DEBUG_ASSERT(!isinf(_clipFar), "Orthogonal cameras do not support an infinite far clip plane");
			_projectionMatrix = Matrix::WithProjectionOrthogonal(_orthoLeft, _orthoRight, _orthoBottom, _orthoTop, _clipNear, _clipFar);
		}
		else
		{
			float tempAspect = _aspect;
			if(std::abs(tempAspect) <= 0.0001)
			{
				Vector2 size = _renderPass->GetFrame().GetSize();
				tempAspect = size.x / size.y;
			}

			_projectionMatrix = Matrix::WithProjectionPerspective(_fov, tempAspect, _clipNear, _clipFar);
		}

		if(_hasCustomNearClipPlane && !(_flags & Flags::Orthogonal))
		{
			Vector3 planePosition = _viewMatrix * _customNearClipPlane.GetPointRelativeTo(_renderOrigin);
			Vector4 planeNormal = _viewMatrix * RN::Vector4(_customNearClipPlane.GetNormal(), 0.0f);
			_projectionMatrix.MakeObliqueNearPlane(Plane::WithPositionNormal(planePosition, RN::Vector3(planeNormal)));
		}

		_inverseProjectionMatrix = _projectionMatrix.GetInverse();

		_dirtyProjection = false;

		_dirtyFrustum = true;
		UpdateFrustum();
	}

	void Camera::UpdateFrustum()
	{
		if(!_dirtyFrustum) return;
		_dirtyFrustum = false;

		bool useSimpleCulling = _flags & Flags::UseSimpleCulling;
		const bool hasInfiniteFarPlane = isinf(_clipFar);
		const Vector3 position = GetRenderPosition();
		Vector3 direction = GetWorldRotation().GetRotatedVector(Vector3(0.0, 0.0, -1.0));

		if(hasInfiniteFarPlane)
		{
			_frustumCenter = position;
			_frustumRadius = FLT_MAX;
		}
		else if(useSimpleCulling)
		{
			_frustumCenter = Vector3(0.0f, 0.0f, _clipFar * 0.5f) + position;
			_frustumRadius = _clipFar * 1.5;
		}

		//far plane is at z=0, near plane z=1 for reverse-z!
		Vector3 pos1 = __ToRender(Vector3(-1.0f, 1.0f, 1.0f));
		Vector3 pos4 = __ToRender(Vector3(1.0f, -1.0f, 1.0));
		Vector3 pos7 = __ToRender(Vector3(1.0f, 1.0f, 1.0f));
		Vector3 pos8 = __ToRender(Vector3(-1.0f, -1.0f, 1.0f));

		if(hasInfiniteFarPlane)
		{
			frustums._frustumLeft = Plane::WithTriangle(position, pos1, pos8, -1.0f, _frustumPlaneOffsets[2]);
			frustums._frustumRight = Plane::WithTriangle(position, pos4, pos7, -1.0f, -_frustumPlaneOffsets[3]);
			frustums._frustumTop = Plane::WithTriangle(position, pos7, pos1, -1.0f, _frustumPlaneOffsets[0]);
			frustums._frustumBottom = Plane::WithTriangle(position, pos8, pos4, -1.0f, -_frustumPlaneOffsets[1]);
		}
		else
		{
			Vector3 pos2 = __ToRender(Vector3(-1.0f, 1.0f, 0.0));
			Vector3 pos3 = __ToRender(Vector3(-1.0f, -1.0f, 0.0));
			Vector3 pos5 = __ToRender(Vector3(1.0f, 1.0f, 0.0));
			Vector3 pos6 = __ToRender(Vector3(1.0f, -1.0f, 0.0));

			if(!useSimpleCulling)
			{
				// Tighter sphere via farthest corner pair among 8 corners
				Vector3 corners[8] = { pos1, pos2, pos3, pos4, pos5, pos6, pos7, pos8 };
				float maxDist2 = 0.0f;
				Vector3 bestA = corners[0], bestB = corners[0];
				for(int i = 0; i < 8; ++i)
				{
					for(int j = i + 1; j < 8; ++j)
					{
						float d2 = corners[i].GetSquaredDistance(corners[j]);
						if(d2 > maxDist2)
						{
							maxDist2 = d2;
							bestA = corners[i];
							bestB = corners[j];
						}
					}
				}
				_frustumCenter = (bestA + bestB) * 0.5f;
				float radius = 0.0f;
				for(int i = 0; i < 8; ++i)
				{
					float d = _frustumCenter.GetDistance(corners[i]);
					if(d > radius) radius = d;
				}
				_frustumRadius = radius;
			}

			frustums._frustumLeft = Plane::WithTriangle(pos1, pos2, pos3, -1.0f, _frustumPlaneOffsets[2]);
			frustums._frustumRight = Plane::WithTriangle(pos4, pos5, pos6, 1.0f, -_frustumPlaneOffsets[3]);
			frustums._frustumTop = Plane::WithTriangle(pos1, pos2, pos5, 1.0f, _frustumPlaneOffsets[0]);
			frustums._frustumBottom = Plane::WithTriangle(pos4, pos3, pos6, -1.0f, -_frustumPlaneOffsets[1]);
			frustums._frustumFar = Plane::WithPositionNormal(position + direction * _clipFar, -direction);
		}
		frustums._frustumNear = Plane::WithPositionNormal(position + direction * std::min(_clipNear, _clipFar), direction);
		if(hasInfiniteFarPlane) frustums._frustumFar = Plane::WithPositionNormal(position + direction * _clipNear, -direction);
	}

	void Camera::EnsureFrustumUpdated()
	{
		const PositionType renderOrigin = ResolveRenderOrigin();
		if(_dirtyPosition || _dirtyProjection || _renderOrigin != renderOrigin
#if RN_ENABLE_LAZY_FRAME_ROOT_TRANSFORMS
			|| _preparedTransformVersion != GetTransformVersion()
#endif
		)
		{
			PrepareRender(renderOrigin);
			return;
		}

		UpdateFrustum();
	}

	Vector3 Camera::__ToRender(const Vector3 &dir)
	{
		Vector4 ndcPos(dir.x, dir.y, dir.z, 1.0f);

		Vector4 vec = _inverseViewMatrix * (_inverseProjectionMatrix * ndcPos);
		vec /= vec.w;

		return Vector3(vec);
	}

	// There should be a much better solution, but at least this works for now
	Vector3 Camera::ToRender(const Vector3 &dir)
	{
		PostUpdate();
		Vector4 ndcPos(dir.x, dir.y, 0.0f, 1.0f);

		Vector4 vec = _inverseProjectionMatrix * ndcPos;
		vec /= vec.w;

		float sign = vec.z < 0.0f ? -1.0f : 1.0f; //Otherwise the division would lose the sign
		vec /= vec.z * sign;
		vec *= dir.z;
		vec.w = 1.0;

		vec = _inverseViewMatrix * vec;

		return Vector3(vec);
	}

	PositionType Camera::ToWorld(const Vector3 &dir)
	{
		const Vector3 renderPosition = ToRender(dir);
		return _renderOrigin + renderPosition;
	}

	void Camera::CreateLightManager(uint16_t maxPackedPointLights, uint16_t maxPackedSpotLights)
	{
		if(_lightManager) return;

		Rect frame = _renderPass->GetFrame();
		uint32 clustersX = std::max<uint32>(1u, static_cast<uint32>(frame.width) / 64u);
		uint32 clustersY = std::max<uint32>(1u, static_cast<uint32>(frame.height) / 64u);
		uint32 clusterXY = std::max<uint32>(1u, clustersX * clustersY);
		uint32 clustersZ = std::max<uint32>(1u, std::min((4000u * 6u) / clusterXY, 32u));
		float zLogFactor = 0.7f;
		_lightManager = new LightManager(clustersX, clustersY, clustersZ, zLogFactor, maxPackedPointLights, maxPackedSpotLights);
	}

	const Vector3 &Camera::GetFrustumCenter()
	{
		EnsureFrustumUpdated();
		return _frustumCenter;
	}

	float Camera::GetFrustumRadius()
	{
		EnsureFrustumUpdated();
		return _frustumRadius;
	}

	void Camera::GetFrustumPlanes(Vector4 *planes) const
	{
		if(!planes) return;

		const_cast<Camera *>(this)->EnsureFrustumUpdated();
		planes[0] = frustums._frustumLeft.GetPlaneVector(PositionType());
		planes[1] = frustums._frustumRight.GetPlaneVector(PositionType());
		planes[2] = frustums._frustumTop.GetPlaneVector(PositionType());
		planes[3] = frustums._frustumBottom.GetPlaneVector(PositionType());
		planes[4] = frustums._frustumNear.GetPlaneVector(PositionType());
		if(isinf(_clipFar))
		{
			Vector3 direction = GetWorldRotation().GetRotatedVector(Vector3(0.0, 0.0, -1.0));
			planes[5] = Vector4(-direction, FLT_MAX);
		}
		else
		{
			planes[5] = frustums._frustumFar.GetPlaneVector(PositionType());
		}
	}

	bool Camera::InFrustum(const PositionType &position, float radius)
	{
		EnsureFrustumUpdated();
		return InRenderFrustum(Vector3(position - _renderOrigin), radius, 0.0f);
	}

	bool Camera::InRenderFrustum(const Vector3 &position, float radius, float maximumRenderDistance) const
	{
		//if(_hasCustomNearClipPlane) return true;

		if(maximumRenderDistance > 0.0f && !IsWithinRenderDistance(position, radius, maximumRenderDistance))
			return false;

		if(!isinf(_clipFar))
		{
			const float combinedRadius = _frustumRadius + radius;
			if(_frustumCenter.GetSquaredDistance(position) > combinedRadius * combinedRadius)
				return false;
		}

		if(_flags & Flags::UseSimpleCulling)
			return true;

		if(frustums._frustumLeft.GetDistance(position) < -radius)
			return false;

		if(frustums._frustumRight.GetDistance(position) < -radius)
			return false;

		if(frustums._frustumTop.GetDistance(position) < -radius)
			return false;

		if(frustums._frustumBottom.GetDistance(position) < -radius)
			return false;

		if(maximumRenderDistance <= 0.0f && frustums._frustumNear.GetDistance(position) < -radius)
			return false;

		if(!isinf(_clipFar) && frustums._frustumFar.GetDistance(position) < -radius)
			return false;

		return true;
	}

	bool Camera::IsWithinRenderDistance(const Vector3 &position, float radius, float maximumRenderDistance) const
	{
		// The near-plane distance is forward view depth, offset by the near clip distance.
		const float nearDistance = frustums._frustumNear.GetDistance(position);
		if(nearDistance < -radius)
			return false;

		const float nearClip = std::min(_clipNear, _clipFar);
		return nearDistance <= maximumRenderDistance + radius - nearClip;
	}

	bool Camera::InFrustum(const Sphere &sphere)
	{
		return InFrustum(sphere, 0.0f);
	}

	bool Camera::InFrustum(const Sphere &sphere, float maximumRenderDistance)
	{
		EnsureFrustumUpdated();
		const Vector3 renderPosition = Vector3(sphere.position - _renderOrigin) + sphere.offset;
		Camera *distanceCamera = GetLODCamera();
		if(maximumRenderDistance <= 0.0f || distanceCamera == this)
			return InRenderFrustum(renderPosition, sphere.radius, maximumRenderDistance);

		if(!InRenderFrustum(renderPosition, sphere.radius, 0.0f))
			return false;

		distanceCamera->EnsureFrustumUpdated();
		const Vector3 distancePosition = Vector3(sphere.position - distanceCamera->_renderOrigin) + sphere.offset;
		return distanceCamera->IsWithinRenderDistance(distancePosition, sphere.radius, maximumRenderDistance);
	}

	bool Camera::InFrustum(const AABB &aabb)
	{
		EnsureFrustumUpdated();
		const Vector3 renderPosition(aabb.position - _renderOrigin);

		// Early-out: camera-centered sphere intersection
		const Vector3 mn = renderPosition + aabb.minExtend;
		const Vector3 mx = renderPosition + aabb.maxExtend;
		const float nx = std::max(mn.x, std::min(_frustumCenter.x, mx.x));
		const float ny = std::max(mn.y, std::min(_frustumCenter.y, mx.y));
		const float nz = std::max(mn.z, std::min(_frustumCenter.z, mx.z));
		const float dx = _frustumCenter.x - nx;
		const float dy = _frustumCenter.y - ny;
		const float dz = _frustumCenter.z - nz;
		const float dist2 = dx*dx + dy*dy + dz*dz;
		if(dist2 > _frustumRadius*_frustumRadius) return false;

		const Plane *planes[6] = { &frustums._frustumLeft, &frustums._frustumRight, &frustums._frustumTop, &frustums._frustumBottom, &frustums._frustumNear, &frustums._frustumFar };
		const int planeCount = isinf(_clipFar) ? 5 : 6;
		for(int i = 0; i < planeCount; ++i)
		{
			const Plane &plane = *planes[i];

			//Pick the corner most in direction of the plane normal
			Vector3 positive;
			positive.x = (plane.GetNormal().x >= 0) ? (renderPosition.x + aabb.maxExtend.x) : (renderPosition.x + aabb.minExtend.x);
			positive.y = (plane.GetNormal().y >= 0) ? (renderPosition.y + aabb.maxExtend.y) : (renderPosition.y + aabb.minExtend.y);
			positive.z = (plane.GetNormal().z >= 0) ? (renderPosition.z + aabb.maxExtend.z) : (renderPosition.z + aabb.minExtend.z);

			if(plane.GetDistance(positive) <= 0.0f)
			{
				return false;
			}
		}

		return true;
	}

	void Camera::AddMultiviewCamera(RN::Camera *camera)
	{
		RN_ASSERT(camera, "Camera cannot be empty");
		RN_ASSERT(camera != this, "Camera cannot use itself for multiview");
		RN_ASSERT(!_multiviewParentCamera, "A multiview child camera cannot own multiview cameras");
		RN_ASSERT(!camera->_multiviewCameras || camera->_multiviewCameras->GetCount() == 0, "A multiview parent camera cannot become a multiview child");
		RN_ASSERT(!camera->_multiviewParentCamera || camera->_multiviewParentCamera == this, "Camera already belongs to another multiview parent");

		if(!_multiviewCameras)
		{
			_multiviewCameras = new RN::Array();
		}

		if(!_multiviewCameras->ContainsObject(camera)) _multiviewCameras->AddObject(camera);
		camera->_multiviewParentCamera = this;
	}

	void Camera::RemoveMultiviewCamera(RN::Camera *camera)
	{
		if(!_multiviewCameras) return;
		_multiviewCameras->RemoveObject(camera);
		if(camera->_multiviewParentCamera == this)
			camera->_multiviewParentCamera = nullptr;
	}

	void Camera::SetFirstSceneNodeMember(IntrusiveList<SceneNode>::Member *member)
	{
		_firstNodeMember = member;
	}

	void Camera::AddRenderNode(SceneNode *node)
	{
		if(!_renderNodes) _renderNodes = new Array();

		_renderNodes->AddObject(node);

		/*size_t insertIndex = 0;
		_renderNodes->Enumerate<SceneNode>([&](SceneNode *oldNode, size_t index, bool &stop) {
			if(oldNode->GetRenderPriority() > node->GetRenderPriority())
			{
				insertIndex = index;
				stop = true;
			}
		});
		_renderNodes->InsertObjectAtIndex(node, insertIndex);*/
	}

	void Camera::RemoveRenderNode(SceneNode *node)
	{
		if(!_renderNodes) return;
		_renderNodes->RemoveObject(node);
	}

	void Camera::TruncateRenderNodes(size_t length)
	{
		if(!_renderNodes) return;
		while(_renderNodes->GetCount() > length)
		{
			_renderNodes->RemoveObjectAtIndex(length);
		}
	}

	void Camera::ClearRenderNodes()
	{
		SafeRelease(_renderNodes);
	}
} // namespace RN
