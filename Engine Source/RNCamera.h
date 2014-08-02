//
//  RNCamera.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CAMERA_H__
#define __RAYNE_CAMERA_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNSceneNode.h"
#include "RNRenderStorage.h"
#include "RNTexture.h"
#include "RNMaterial.h"
#include "RNRect.h"
#include "RNPlane.h"
#include "RNColor.h"
#include "RNModel.h"
#include "RNSphere.h"
#include "RNEnum.h"

namespace RN
{
	class Light;
	class Camera;
	class Renderer;
	class LightManager;
	
	class RenderStage
	{
	public:
		friend class Renderer;
		
		enum class Mode
		{
			ReRender,
			ReUseConnection,
			ReUsePreviousStage,
			ReUsePipeline,
			ReUseCamera,
			
			ReRender_NoRemoval,
			ReUseConnection_NoRemoval,
			ReUsePreviousStage_NoRemoval,
			ReUsePipeline_NoRemoval,
			ReUseCamera_NoRemoval
		};
		
		RNAPI RenderStage(Camera *camera, Camera *conenction, Mode mode);
		RNAPI RenderStage(const RenderStage &other);
		RNAPI ~RenderStage();
		
		RNAPI void Connect(Camera *other);
		
		RNAPI Camera *GetConnection() const { return _connection; }
		RNAPI Camera *GetCamera() const { return _camera; }
		RNAPI Mode GetMode() const { return _mode; }
		
	private:
		void InsertCamera(class Camera *camera);
		void RemoveCamera(class Camera *camera);
		
		Camera *_connection;
		Camera *_camera;
		
		Mode _mode;
	};
	
	class PostProcessingPipeline : public Object
	{
	public:
		friend class Camera;
		friend class Renderer;
		
		RNAPI PostProcessingPipeline(const std::string &name, int32 priority);
		RNAPI ~PostProcessingPipeline();
		
		RNAPI RenderStage *AddStage(Camera *camera, RenderStage::Mode mode);
		RNAPI RenderStage *AddStage(Camera *camera, Camera *connection, RenderStage::Mode mode);
		
		RNAPI const std::vector<RenderStage>& GetStages() const { return stages; }
		RNAPI RenderStage *GetLastStage() { return &stages[stages.size() - 1]; }
		
	protected:
		RNAPI virtual void Initialize();
		RNAPI virtual void PushUpdate(Camera *source, float delta);
		
		Camera *host;
		std::vector<RenderStage> stages;
		
	private:
		void PostUpdate(Camera *source, const Vector3 &position, const Quaternion &rotation, const Rect &frame);
		void PushProjectionUpdate(Camera *source);
		
		std::string _name;
		int32 _priority;
		
		RNDeclareMeta(PostProcessingPipeline)
	};
	
	class Camera : public SceneNode
	{
	public:
		friend class Renderer;
		friend class RenderStage;
		friend class PostProcessingPipeline;
		friend class Light;
		friend class World;
		
		struct Flags : public Enum<uint32>
		{
			Flags()
			{}
			Flags(int value) :
				Enum(value)
			{}
			
			enum
			{
				UpdateAspect       = (1 << 0),
				UpdateStorageFrame = (1 << 1),
				
				NoSky        = (1 << 5),
				NoSorting    = (1 << 6),
				NoRender     = (1 << 7),
				NoFlush      = (1 << 8),
				NoDepthWrite = (1 << 9),
				ForceFlush   = (1 << 10),
				
				InheritPosition   = (1 << 12),
				InheritFrame      = (1 << 13),
				InheritProjection = (1 << 14),
				
				Fullscreen = (1 << 20),
				Orthogonal = (1 << 21),
				Hidden     = (1 << 22),
				
				UseFog          = (1 << 24),
				UseClipPlanes   = (1 << 25),
				BlendedBlitting = (1 << 26),
				
				Defaults = (Fullscreen | UpdateAspect | UpdateStorageFrame | UseFog),
				Inherit  = (InheritFrame | InheritPosition | InheritProjection)
			};
		};
		
		struct ClearMask : public Enum<uint32>
		{
			ClearMask()
			{}
			ClearMask(int value) :
				Enum(value)
			{}
			
			enum
			{
				Color   = (1 << 0),
				Depth   = (1 << 1),
				Stencil = (1 << 2)
			};
		};
		
		struct ColorMask : public Enum<uint32>
		{
			ColorMask()
			{}
			ColorMask(int value) :
				Enum(value)
			{}
			
			enum
			{
				Red    = (1 << 0),
				Green  = (1 << 1),
				Blue   = (1 << 2),
				Alpha  = (1 << 3)
			};
		};
		
		struct RenderGroups : public Enum<uint32>
		{
			RenderGroups()
			{}
			RenderGroups(int value) :
				Enum(value)
			{}
			
			enum
			{
				Group0 = (1 << 0),
				Group1 = (1 << 1),
				Group2 = (1 << 2),
				Group3 = (1 << 3),
				Group4 = (1 << 4),
				Group5 = (1 << 5),
				Group7 = (1 << 7),
				Group8 = (1 << 8),
				Group9 = (1 << 9)
			};
		};
		
		enum class BlitMode
		{
			Stretched,
			StretchedVertical,
			StretchedHorizontal,
			Unstretched
		};
		
		RNAPI Camera();
		RNAPI Camera(const Vector2 &size);
		
		RNAPI Camera(const Vector2 &size, Texture *target);
		RNAPI Camera(const Vector2 &size, Texture *target, Flags flags);
		RNAPI Camera(const Vector2 &size, Texture *target, Flags flags, RenderStorage::BufferFormat format);
		
		RNAPI Camera(const Vector2 &size, Texture::Format targetFormat);
		RNAPI Camera(const Vector2 &size, Texture::Format targetFormat, Flags flags);
		RNAPI Camera(const Vector2 &size, Texture::Format targetFormat, Flags flags, RenderStorage::BufferFormat format, float scaleFactor=0.0f);
		
		RNAPI Camera(const Vector2 &size, RenderStorage *storage, Flags flags, float scaleFactor=0.0f);
		RNAPI ~Camera() override;
		
		RNAPI void DidUpdate(ChangeSet changeSet) override;
		
		RNAPI void PrepareForRendering(Renderer *renderer);
		
		RNAPI void SetFrame(const Rect &frame);
		RNAPI void SetRenderingFrame(const Rect &offset);
		RNAPI void SetFlags(Flags flags);
		RNAPI void SetClearColor(const Color &color);
		RNAPI void SetMaterial(Material *material);
		RNAPI void SetRenderStorage(RenderStorage *storage);
		RNAPI void SetLightManager(LightManager *lightManager);
		RNAPI void SetClearMask(ClearMask mask);
		RNAPI void SetColorMask(ColorMask mask);
		RNAPI void SetSky(Model *sky);
		RNAPI void SetLODCamera(Camera *camera);
		RNAPI void SetPriority(int32 priority);
		RNAPI void SetBlitShader(Shader *shader);
		RNAPI void SetBlitMode(BlitMode mode);
		RNAPI void SetFOV(float fov);
		RNAPI void SetAspectRatio(float ratio);
		RNAPI void SetClipNear(float near);
		RNAPI void SetClipFar(float far);
		RNAPI void SetFogColor(Color color);
		RNAPI void SetFogNear(float near);
		RNAPI void SetFogFar(float far);
		RNAPI void SetAmbientColor(Color color);
		RNAPI void SetClipPlane(const Plane &clipPlane);
		RNAPI void SetRenderGroups(RenderGroups groups);
		RNAPI void SetOrthogonalFrustum(float top, float bottom, float left, float right);
		RNAPI void SetProjectionMatrix(const Matrix &projectionMatrix);
		
		RNAPI void Update(float delta) override;
		RNAPI void UpdateEditMode(float delta) override;
		
		RNAPI Vector3 ToWorld(const Vector3 &dir);
		
		RNAPI virtual bool InFrustum(const Vector3 &position, float radius);
		RNAPI virtual bool InFrustum(const Sphere &sphere);
		RNAPI virtual bool InFrustum(const AABB &aabb);
		
		RNAPI bool IsVisibleInCamera(Camera *camera) override;
		
		RNAPI const Vector3 &GetFrustumCenter();
		RNAPI float GetFrustumRadius();
		
		RenderStorage *GetStorage() const { return _storage; }
		const Color &GetClearColor() const { return _clearColor; }
		RNAPI const Rect &GetFrame();
		RNAPI Rect GetRenderingFrame();
		Material *GetMaterial() const { return _material; }
		Flags GetFlags() const { return _flags; }
		Camera *GetLODCamera() const { return _lodCamera ? _lodCamera : const_cast<Camera *>(this); }
		Model *GetSky() const { return _sky; }
		LightManager *GetLightManager();
		int32 GetPriority() const { return _priority; }
		Shader *GetBlitShader() const { return _blitShader; }
		BlitMode GetBlitMode() const { return _blitMode; }
		RenderGroups GetRenderGroups() const { return _renderGroups; }
		float GetFOV() const { return _fov; }
		float GetAspectRatio() const { return _aspect; }
		float GetClipNear() const { return _clipNear; }
		float GetClipFar() const { return _clipFar; }
		const Color &GetFogColor() const { return _fogColor; }
		float GetFogNear() const { return _fogNear; }
		float GetFogFar() const { return _fogFar; }
		const Color &GetAmbientColor() const { return _ambient; }
		const Plane &GetClipPlane() const { return _clipPlane; }
		const Matrix &GetProjectionMatrix() const { return _projectionMatrix; }
		const Matrix &GetInverseProjectionMatrix() const { return _inverseProjectionMatrix; }
		const Matrix &GetViewMatrix() const { return _viewMatrix; }
		const Matrix &GetInverseViewMatrix() const { return _inverseViewMatrix; }
		
		uint32 GetRenderTargetCount() const { return (uint32)_storage->_renderTargets->GetCount(); }
		Texture *GetRenderTarget(uint32 index=0) const { return _storage->_renderTargets->GetObjectAtIndex<Texture>(index); }
		
		bool HasDepthbuffer() const { return _storage->HasDepthbuffer(); }
		bool HasStencilbuffer() const { return _storage->HasStencilbuffer(); }
		
		RNAPI PostProcessingPipeline *AddPostProcessingPipeline(const std::string &name, int32 priority);
		RNAPI PostProcessingPipeline *GetPostProcessingPipeline(const std::string &name);
		RNAPI void AddPostProcessingPipeline(PostProcessingPipeline *pipeline);
		RNAPI void RemovePostProcessingPipeline(PostProcessingPipeline *pipeline);
		
		const std::vector<PostProcessingPipeline *>& GetPostProcessingPipelines() const { return _PPPipelines; }
		
		RNAPI class Hit CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode) override;
		
	private:
		void PostUpdate();
		void UpdateProjection();
		void UpdateFrustum();
		
		Vector3 __ToWorld(const Vector3 &dir);
		Matrix MakeShadowSplit(Camera *camera, Light *light, float near, float far);
		void Initialize();
		
		Rect _frame;
		Rect _renderingFrame;
		Color _clearColor;
		Flags _flags;
		ColorMask _colorMask;
		GLuint _clearMask;
		BlitMode _blitMode;
		float _scaleFactor;
		bool _fixedScaleFactor;
		bool _dirtyProjection;
		bool _dirtyFrustum;
		int32 _priority;
		
		Vector3 _frustumCenter;
		float _frustumRadius;
		
		struct
		{
			Plane _frustumLeft;
			Plane _frustumRight;
			Plane _frustumTop;
			Plane _frustumBottom;
			Plane _frustumFar;
			Plane _frustumNear;
		} frustrums;
		
		float _fov;
		float _aspect;
		float _clipNear;
		float _clipFar;
		
		float _fogNear;
		float _fogFar;
		
		Color _fogColor;
		Color _ambient;
		
		Plane _clipPlane;
		RenderGroups _renderGroups;
		LightManager *_lightManager;
		
		Matrix _projectionMatrix;
		Matrix _inverseProjectionMatrix;
		Matrix _viewMatrix;
		Matrix _inverseViewMatrix;
		
		float _orthoLeft;
		float _orthoRight;
		float _orthoTop;
		float _orthoBottom;
		
		bool _prefersLightManager;
		
		Shader *_blitShader;
	
		Material *_material;
		RenderStorage *_storage;
		Camera *_lodCamera;
		Model *_sky;
		
		uint32 _stageCount;
		
		std::vector<PostProcessingPipeline *> _PPPipelines;
		std::map<std::string, PostProcessingPipeline *> _namedPPPipelines;
		
		RNDeclareMeta(Camera)
	};
	
	RNObjectClass(Camera)
}

#endif /* __RAYNE_CAMERA_H__ */
