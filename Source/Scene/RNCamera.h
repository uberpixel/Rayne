//
//  RNCamera.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_CAMERA_H__
#define __RAYNE_CAMERA_H__

#include "../Base/RNBase.h"
#include "../Math/RNPlane.h"
#include "../Math/RNRect.h"
#include "../Rendering/RNRenderPass.h"
#include "RNSceneNode.h"

namespace RN
{
	class Window;
	class Light;
	class LightManager;
	class Camera : public SceneNode
	{
	public:
		friend class Scene;
		friend class Light;

		RN_OPTIONS(Flags, uint32,
				   SortTransparentBackToFront = (1 << 5),
				   SortFrontToBack = (1 << 6),
				   NoRender = (1 << 7),
				   NoDepthWrite = (1 << 8),
				   NoOcclusionCulling = (1 << 9),
				   SortInstancable = (1 << 10),

				   InheritPosition = (1 << 12),
				   InheritFrame = (1 << 13),
				   InheritProjection = (1 << 14),

				   RenderEarly = (1 << 19),
				   RenderLate = (1 << 20),

				   Orthogonal = (1 << 21),
				   Hidden = (1 << 22),

				   UseFog = (1 << 24),
				   UseSimpleCulling = (1 << 26),
				   UseUIFastPath = (1 << 27),

				   Defaults = (UseFog | SortTransparentBackToFront)); // | SortFrontToBack));


		RNAPI Camera(RenderPass *renderPass = nullptr);
		RNAPI Camera(const Vector2 &size);
		RNAPI ~Camera();

		RNAPI void DidUpdate(ChangeSet changeSet) override;

		RNAPI void SetRenderPass(RenderPass *renderPass);
		RNAPI void SetRootFramePass(FramePass *framePass); // Passing nullptr clears the override and uses GetRenderPass() as the root.
		RNAPI void SetFlags(Flags flags);
		RNAPI void SetLODCamera(Camera *camera);
		RNAPI void SetPriority(int32 priority);
		RNAPI void SetFOV(float fov);
		RNAPI void SetAspectRatio(float ratio);
		RNAPI void SetClipNear(float near);
		RNAPI void SetClipFar(float far);
		RNAPI void SetClipFarUnlimited();
		RNAPI void SetReferenceFar(float far);
		RNAPI void SetFogColor0(Color color);
		RNAPI void SetFogColor1(Color color);
		RNAPI void SetFogNear(float near);
		RNAPI void SetFogFar(float far);
		RNAPI void SetAmbientColor(Color color);
		RNAPI void SetCustomData(const Vector4 &data);
		RNAPI void SetCustomNearClipPlane(const Plane &clipPlane, bool enabled = true);
		RNAPI void SetOrthogonalFrustum(float top, float bottom, float left, float right);
		RNAPI void SetProjectionMatrix(const Matrix &projectionMatrix);

		RNAPI void SetFrustumPlaneOffset(float topOffset, float bottomOffset, float leftOffset, float rightOffset);

		RNAPI void AddMultiviewCamera(RN::Camera *camera);
		RNAPI void RemoveMultiviewCamera(RN::Camera *camera);

		RNAPI void CreateLightManager(uint16_t maxPackedPointLights = 256, uint16_t maxPackedSpotLights = 256);

		RNAPI void Update(float delta) override;
		RNAPI void PostUpdate();

		RNAPI Vector3 ToRender(const Vector3 &dir);
		RNAPI PositionType ToWorld(const Vector3 &dir);

		RNAPI virtual bool InFrustum(const PositionType &position, float radius);
		RNAPI virtual bool InFrustum(const Sphere &sphere);
		RNAPI bool InFrustum(const Sphere &sphere, float maximumRenderDistance);
		RNAPI virtual bool InFrustum(const AABB &aabb);

		RNAPI const Vector3 &GetFrustumCenter();
		RNAPI float GetFrustumRadius();
		RNAPI void GetFrustumPlanes(Vector4 *planes) const;

		RenderPass *GetRenderPass() const { return _renderPass; }
		FramePass *GetRootFramePass() const { return _rootFramePass ? _rootFramePass : _renderPass; }
		Flags GetFlags() const { return _flags; }
		Camera *GetLODCamera() const { return _lodCamera ? _lodCamera : const_cast<Camera *>(this); }
		LightManager *GetLightManager() const { return _lightManager; }
		int32 GetPriority() const { return _priority; }
		float GetFOV() const { return _fov; }
		float GetAspectRatio() const { return _aspect; }
		float GetClipNear() const { return _clipNear; }
		float GetClipFar() const { return _clipFar; }
		float GetReferenceFar() const
		{
			if(_referenceFar != 0.0f) return _referenceFar;
			if(!isinf(_clipFar)) return _clipFar;
			return 500.0f;
		}
		const Color &GetFogColor0() const { return _fogColor0; }
		const Color &GetFogColor1() const { return _fogColor1; }
		float GetFogNear() const { return _fogNear; }
		float GetFogFar() const { return _fogFar; }
		const Color &GetAmbientColor() const { return _ambient; }
		const Vector4 &GetCustomData() const { return _customData; }
		const Plane &GetCustomNearClipPlane() const { return _customNearClipPlane; }
		const Matrix &GetProjectionMatrix() const { return _projectionMatrix; }
		const Matrix &GetInverseProjectionMatrix() const { return _inverseProjectionMatrix; }
		const Matrix &GetViewMatrix() const { return _viewMatrix; }
		const Matrix &GetInverseViewMatrix() const { return _inverseViewMatrix; }
		const PositionType &GetRenderOrigin() const { return _renderOrigin; }
		Vector3 GetRenderPosition() const { return Vector3(GetWorldPosition() - _renderOrigin); }
		const RN::Array *GetMultiviewCameras() const { return _multiviewCameras; }
		bool GetIsMultiviewCamera() const { return _multiviewParentCamera != nullptr; }

		RNAPI void AddRenderNode(SceneNode *node);
		RNAPI void RemoveRenderNode(SceneNode *node);
		RNAPI void TruncateRenderNodes(size_t length);
		RNAPI void ClearRenderNodes();
		RNAPI const Array *GetRenderNodes() const { return _renderNodes; }

		RNAPI void SetFirstSceneNodeMember(IntrusiveList<SceneNode>::Member *member); //If this is set, it has to be reset if the member is removed from the scene!

		//TODO: Make private but keep accessible to user made scene implementations
		IntrusiveList<Camera>::Member _cameraSceneEntry;
		IntrusiveList<SceneNode>::Member *_firstNodeMember; //Start rendering with this node if it is set! Useful when rendering UI into a separate camera, skipping a lot of work.

	private:
		void UpdateProjection();
		void UpdateFrustum();
		void EnsureFrustumUpdated();
		void PrepareRender(const PositionType &renderOrigin);
		bool IsWithinRenderDistance(const Vector3 &position, float radius, float maximumRenderDistance) const;
		bool InRenderFrustum(const Vector3 &position, float radius, float maximumRenderDistance) const;

		Vector3 __ToRender(const Vector3 &dir);
		PositionType ResolveRenderOrigin() const;
		Matrix MakeShadowSplit(Camera *camera, const Quaternion &shadowRotation, float cameraDistanceToCenter, float near, float far);
		void Initialize();

		Flags _flags;
		float _scaleFactor;
		bool _fixedScaleFactor;
#if RN_ENABLE_LAZY_FRAME_ROOT_TRANSFORMS
		uint64 _preparedTransformVersion;
#endif
		bool _dirtyPosition;
		bool _dirtyProjection;
		bool _dirtyFrustum;
		bool _hasCustomNearClipPlane;
		bool _hasExplicitProjectionMatrix;
		int32 _priority;

		Vector3 _frustumCenter;
		float _frustumRadius;

		float _frustumPlaneOffsets[4];

		struct
		{
			Plane _frustumLeft;
			Plane _frustumRight;
			Plane _frustumTop;
			Plane _frustumBottom;
			Plane _frustumFar;
			Plane _frustumNear;
		} frustums;

		float _fov;
		float _aspect;
		float _clipNear;
		float _clipFar;
		float _referenceFar;

		float _fogNear;
		float _fogFar;

		Color _fogColor0;
		Color _fogColor1;
		Color _ambient;
		Vector4 _customData;

		Plane _customNearClipPlane;
		LightManager *_lightManager;

		Matrix _projectionMatrix;
		Matrix _explicitProjectionMatrix;
		Matrix _inverseProjectionMatrix;
		Matrix _viewMatrix;
		Matrix _inverseViewMatrix;
		PositionType _renderOrigin;
#if RN_ENABLE_UNIVERSE_SCALE && RN_ENABLE_LAZY_FRAME_ROOT_TRANSFORMS
		mutable PositionType _cachedRenderOrigin;
		mutable uint64 _cachedRenderOriginVersion;
#endif

		float _orthoLeft;
		float _orthoRight;
		float _orthoTop;
		float _orthoBottom;

		RenderPass *_renderPass;
		FramePass *_rootFramePass;
		Camera *_lodCamera;

		Array *_multiviewCameras;
		Camera *_multiviewParentCamera;

		Array *_renderNodes; //If this is set, only the scene nodes in this list will be rendered by the camera and everything else will be ignored, skipping occlusion culling. Useful when there is only a small set of things that can be visible in a camera

		__RNDeclareMetaInternal(Camera);
	};
} // namespace RN


#endif /* __RAYNE_CAMERA_H__ */
