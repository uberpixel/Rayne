//
//  RNCamera.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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

namespace RN
{
	class Light;
	class Camera;
	class Renderer;
	class LightManager;
	
	class RenderStage
	{
	friend class Renderer;
	public:
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
		
		RenderStage(Camera *camera, Camera *conenction, Mode mode);
		RenderStage(const RenderStage& other);
		~RenderStage();
		
		void Connect(Camera *other);
		
		Camera *GetConnection() const { return _connection; }
		Camera *GetCamera() const { return _camera; }
		Mode GetMode() const { return _mode; }
		
	private:
		void InsertCamera(class Camera *camera);
		void RemoveCamera(class Camera *camera);
		
		Camera *_connection;
		Camera *_camera;
		
		Mode _mode;
	};
	
	class PostProcessingPipeline
	{
	friend class Camera;
	friend class Renderer;
	public:
		PostProcessingPipeline(const std::string& name);
		virtual ~PostProcessingPipeline();
		
		RenderStage *AddStage(Camera *camera, RenderStage::Mode mode);
		RenderStage *AddStage(Camera *camera, Camera *connection, RenderStage::Mode mode);
		
		const std::vector<RenderStage>& GetStages() const { return stages; }
		RenderStage *GetLastStage() { return &stages[stages.size() - 1]; }
		
	protected:
		virtual void Initialize();
		virtual void PushUpdate(Camera *source, float delta);
		
		Camera *host;
		std::vector<RenderStage> stages;
		
	private:
		void PostUpdate(Camera *source, const Vector3& position, const Quaternion& rotation, const Rect& frame);
		void PushProjectionUpdate(Camera *source);
		
		std::string _name;
	};
	
	
	class DownsamplePostProcessingPipeline : public PostProcessingPipeline
	{
	public:
		DownsamplePostProcessingPipeline(const std::string& name, Camera *camera, Texture *texture, Shader *firstShader, Shader *shader, TextureParameter::Format format);
		~DownsamplePostProcessingPipeline();
		
		Texture *GetLastTarget() { return _lastTarget; }
		
	protected:
		void Initialize() override;
		void PushUpdate(Camera *camera, float delta) override;
		
	private:
		void UpdateStages();
		void RecreateStages();
		
		int _level;
		Rect _frame;
		
		Camera *_camera;
		TextureParameter::Format _format;
		Texture *_texture;
		Texture *_lastTarget;
		Shader *_firstShader;
		Shader *_shader;
	};
	
	
	class Camera : public SceneNode
	{
	public:
		friend class Renderer;
		friend class RenderStage;
		friend class PostProcessingPipeline;
		
		enum
		{
			FlagNoSky = (1 << 0),
			FlagUpdateAspect = (1 << 1),
			FlagFullscreen = (1 << 2),
			FlagNoClear = (1 << 3),
			FlagInheritPosition = (1 << 4),
			FlagInheritFrame = (1 << 5),
			FlagInheritProjection = (1 << 6),
			FlagUpdateStorageFrame = (1 << 7),
			FlagOrthogonal = (1 << 8),
			FlagHidden = (1 << 9),
			FlagNoSorting = (1 << 10),
			FlagNoRender = (1 << 11),
			FlagForceFlush = (1 << 12),
			FlagNoLights = (1 << 13),
			
			FlagDefaults = (FlagFullscreen | FlagUpdateAspect | FlagUpdateStorageFrame),
			FlagInherit = (FlagInheritFrame | FlagInheritPosition | FlagInheritProjection)
		};
		typedef uint32 Flags;
		
		enum
		{
			ClearFlagColor = (1 << 0),
			ClearFlagDepth = (1 << 1),
			ClearFlagStencil = (1 << 2)
		};
		typedef uint32 ClearFlags;
		
		enum
		{
			ColorFlagRed = (1 << 0),
			ColorFlagGreen = (1 << 1),
			ColorFlagBlue = (1 << 2),
			ColorFlagAlpha = (1 << 3)
		};
		typedef uint32 ColorFlags;
		
		enum
		{
			RenderGroup0 = (1 << 0),
			RenderGroup1 = (1 << 1),
			RenderGroup2 = (1 << 2),
			RenderGroup3 = (1 << 3),
			RenderGroup4 = (1 << 4),
			RenderGroup5 = (1 << 5),
			RenderGroup7 = (1 << 7),
			RenderGroup8 = (1 << 8),
			RenderGroup9 = (1 << 9)
		};
		typedef uint32 RenderGroups;
		
		enum class BlitMode
		{
			Stretched,
			StretchedVertical,
			StretchedHorizontal,
			Unstretched
		};
		
		RNAPI Camera(const Vector2& size);
		
		RNAPI Camera(const Vector2& size, Texture *target);
		RNAPI Camera(const Vector2& size, Texture *target, Flags flags);
		RNAPI Camera(const Vector2& size, Texture *target, Flags flags, RenderStorage::BufferFormat format);
		
		RNAPI Camera(const Vector2& size, TextureParameter::Format targetFormat);
		RNAPI Camera(const Vector2& size, TextureParameter::Format targetFormat, Flags flags);
		RNAPI Camera(const Vector2& size, TextureParameter::Format targetFormat, Flags flags, RenderStorage::BufferFormat format, float scaleFactor=0.0f);
		
		RNAPI Camera(const Vector2& size, RenderStorage *storage, Flags flags, float scaleFactor=0.0f);
		
		RNAPI virtual ~Camera();
		
		RNAPI void Bind();
		RNAPI void Unbind();
		RNAPI void PrepareForRendering(Renderer *renderer);
		
		RNAPI void SetFrame(const Rect& frame);
		RNAPI void SetRenderingFrame(const Rect& offset);
		RNAPI void SetFlags(Flags flags);
		RNAPI void SetClearColor(const Color& color);
		RNAPI void SetMaterial(Material *material);
		RNAPI void SetRenderStorage(RenderStorage *storage);
		RNAPI void SetClearMask(ClearFlags mask);
		RNAPI void SetColorMask(ColorFlags mask);
		RNAPI void SetAllowsDepthWrite(bool flag);
		RNAPI void SetSkyCube(Model *skycube);
		RNAPI void SetMaxLightsPerTile(size_t lights);
		RNAPI void SetCameraFlags(Flags flags);
		RNAPI void SetLODCamera(Camera *camera);
		RNAPI void SetPriority(int32 priority);
		RNAPI void SetUseBlending(bool useBlending);
		RNAPI void SetDrawFramebufferShader(Shader *shader);
		RNAPI void SetBlitMode(BlitMode mode);
		
		RNAPI Matrix MakeShadowSplit(Camera *camera, Light *light, float near, float far);
		
		RNAPI void Update(float delta);
		RNAPI void PostUpdate();
		RNAPI void UpdateProjection();
		
		RNAPI Vector3 ToWorld(const Vector3& dir);
		RNAPI Vector3 ToWorldZ(const Vector3& dir);
		RNAPI void UpdateFrustum();
		RNAPI virtual bool InFrustum(const Vector3& position, float radius);
		RNAPI virtual bool InFrustum(const Sphere& sphere);
		RNAPI virtual bool InFrustum(const AABB& aabb);
		
		RNAPI virtual bool IsVisibleInCamera(Camera *camera);
		
		RNAPI virtual Hit CastRay(const Vector3 &position, const Vector3 &direction) {return Hit();}
		
		const Vector3& GetFrustumCenter() const { return _frustumCenter; }
		float const GetFrustumRadius() const { return _frustumRadius; }
		
		RenderStorage *GetStorage() const { return _storage; }
		const Color& GetClearColor() const { return _clearColor; }
		const Rect& GetFrame();
		Rect GetRenderingFrame();
		Material *GetMaterial() const { return _material; }
		Flags GetFlags() const { return _flags; }
		Camera *GetLODCamera() const { return _lodCamera; }
		int32 GetPriority() const { return _priority; }
		bool UseBlending() const { return _blend; }
		Shader *GetDrawFramebufferShader() const { return _blitShader; }
		BlitMode GetBlitMode() const { return _blitMode; }
		
		Vector3 GetLightTiles() const { return _lightTiles; }
		void SetLightTiles(const Vector3 &size) { _lightTiles = size; }
		Model *GetSkyCube() const { return _skycube; }
		
		uint32 GetRenderTargetCount() const { return (uint32)_storage->_renderTargets->GetCount(); }
		Texture *GetRenderTarget(uint32 index=0) const { return _storage->_renderTargets->GetObjectAtIndex<Texture>(index); }
		
		bool HasDepthbuffer() const { return _storage->HasDepthbuffer(); }
		bool HasStencilbuffer() const { return _storage->HasStencilbuffer(); }
		
		bool AllowsDepthWrite() const { return _allowDepthWrite; }
		
		float *GetDepthArray();
		size_t GetMaxLightsPerTile() const { return _maxLights; }
		
		PostProcessingPipeline *AddPostProcessingPipeline(const std::string& name);
		PostProcessingPipeline *PostProcessingPipelineWithName(const std::string& name);
		void AttachPostProcessingPipeline(PostProcessingPipeline *pipeline);
		void RemovePostProcessingPipeline(PostProcessingPipeline *pipeline);
		
		const std::vector<PostProcessingPipeline *>& GetPostProcessingPipelines() const { return _PPPipelines; }
		
		RNAPI virtual class Hit CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode = Hit::HitMode::IgnoreNone);
		
		float fov;
		float aspect;
		float clipnear;
		float clipfar;
		
		float ortholeft;
		float orthoright;
		float orthotop;
		float orthobottom;
		
		bool usefog;
		float fognear;
		float fogfar;
		Color fogcolor;
		
		bool useclipplane;
		Vector4 clipplane;
		
		LightManager *lightManager;
		
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		
		RenderGroups renderGroup;
		
	protected:
		void Initialize();
		
	private:
		Rect _frame;
		Rect _renderingFrame;
		Color _clearColor;
		Flags _flags;
		ColorFlags _colorMask;
		GLuint _clearMask;
		BlitMode _blitMode;
		float _scaleFactor;
		bool _fixedScaleFactor;
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
		
		Vector3 _lightTiles;
		
		bool _allowDepthWrite;
		bool _blend;
		
		Shader *_blitShader;
		
		Material *_material;
		RenderStorage *_storage;
		Camera *_lodCamera;
		
		Model *_skycube;
		
		size_t _maxLights;
		uint32 _stageCount;
		
		std::vector<PostProcessingPipeline *> _PPPipelines;
		std::map<std::string, PostProcessingPipeline *> _namedPPPipelines;
		
		RNDefineMeta(Camera, SceneNode)
	};
	
	class CubemapCamera : public Camera
	{
	public:
	/*	RNAPI CubemapCamera(const Vector2& size) : Camera(size){}
		
		RNAPI CubemapCamera(const Vector2& size, Texture *target) : Camera(size, target){}
		RNAPI CubemapCamera(const Vector2& size, Texture *target, Flags flags) : Camera(size, target, flags){}
		RNAPI CubemapCamera(const Vector2& size, Texture *target, Flags flags, RenderStorage::BufferFormat format) : Camera(size, target, flags, format){}
		
		RNAPI CubemapCamera(const Vector2& size, TextureParameter::Format targetFormat) : Camera(size, targetFormat){}
		RNAPI CubemapCamera(const Vector2& size, TextureParameter::Format targetFormat, Flags flags) : Camera(size, targetFormat, flags){}
		RNAPI CubemapCamera(const Vector2& size, TextureParameter::Format targetFormat, Flags flags, RenderStorage::BufferFormat format, float scaleFactor=0.0f) : Camera(size, targetFormat, flags, format, scaleFactor){}*/
		
		RNAPI CubemapCamera(const Vector2& size, RenderStorage *storage, Flags flags, float scaleFactor=0.0f) : Camera(size, storage, flags, scaleFactor){}
		
		bool InFrustum(const Vector3& position, float radius) override;
	};
}

#endif /* __RAYNE_CAMERA_H__ */
