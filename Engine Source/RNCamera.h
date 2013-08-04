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
			ReUseCamera
		};
		
		RenderStage(Camera *camera, Camera *conenction, Mode mode);
		RenderStage(const RenderStage& other);
		~RenderStage();
		
		void Connect(Camera *other);
		
		Camera *Connection() const { return _connection; }
		Camera *Camera() const { return _camera; }
		Mode StageMode() const { return _mode; }
		
	private:
		void InsertCamera(class Camera *camera);
		void RemoveCamera(class Camera *camera);
		
		class Camera *_connection;
		class Camera *_camera;
		
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
		
		const std::vector<RenderStage>& Stages() const { return stages; }
		RenderStage *LastStage() { return &stages[stages.size() - 1]; }
		
	protected:
		virtual void Initialize();
		virtual void PushUpdate(float delta);
		
		Camera *host;
		std::vector<RenderStage> stages;
		
	private:
		void PostUpdate(const Vector3& position, const Quaternion& rotation, const Rect& frame);
		void PushProjectionUpdate(Camera *source);
		
		std::string _name;
	};
	
	
	class DownsamplePostProcessingPipeline : public PostProcessingPipeline
	{
	public:
		DownsamplePostProcessingPipeline(const std::string& name, Camera *camera, Texture *texture, Shader *firstShader, Shader *shader, TextureParameter::Format format);
		~DownsamplePostProcessingPipeline();
		
		Texture *LastTarget() { return _lastTarget; }
		
	protected:
		void Initialize() override;
		void PushUpdate(float delta) override;
		
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
	friend class Renderer;
	friend class RenderStage;
	friend class PostProcessingPipeline;
	public:
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
		RNAPI void PrepareForRendering();
		
		RNAPI void SetFrame(const Rect& frame);
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
		RNAPI void SetUseInstancing(bool activate);
		RNAPI void SetPriority(int32 priority);
		RNAPI void SetUseBlending(bool useBlending);
		RNAPI void SetDrawFramebufferShader(Shader *shader);
		RNAPI void SetRenderingOffset(const Vector2& offset);
		
		RNAPI Matrix MakeShadowSplit(Camera *camera, Light *light, float near, float far);
		RNAPI void ActivateTiledLightLists(Texture *depthTiles);
		Texture *DepthTiles() { return _depthTiles; }
		RNAPI bool UseInstancing() { return _useInstancing; }
		
		RNAPI void Update(float delta);
		RNAPI void PostUpdate();
		RNAPI void UpdateProjection();
		
		RNAPI Vector3 ToWorld(const Vector3& dir);
		RNAPI Vector3 ToWorldZ(const Vector3& dir);
		RNAPI void UpdateFrustum();
		RNAPI bool InFrustum(const Vector3& position, float radius);
		RNAPI bool InFrustum(const Sphere& sphere);
		RNAPI bool InFrustum(const AABB& aabb);
		
		RNAPI bool RequiresRendering() const;
		
		RNAPI virtual bool IsVisibleInCamera(Camera *camera);
		
		const Vector3& FrustumCenter() const { return _frustumCenter; }
		float const FrustumRadius() const { return _frustumRadius; }
		
		RenderStorage *Storage() const { return _storage; }
		const Color& ClearColor() const { return _clearColor; }
		const Rect& Frame();
		Material *Material() const { return _material; }
		Flags CameraFlags() const { return _flags; }
		Camera *LODCamera() const { return _lodCamera; }
		int32 Priority() const { return _priority; }
		bool UseBlending() const { return _blend; }
		Shader *DrawFramebufferShader() const { return _blitShader; }
		const Vector2& RenderingOffset() const { return _renderingOffset; }
		
		Vector2 LightTiles() const { return _lightTiles; }
		void SetLightTiles(const Vector2 &size) { _lightTiles = size; }
		Model *SkyCube() const { return _skycube; }
		
		uint32 RenderTargets() const { return (uint32)_storage->_renderTargets->Count(); }
		Texture *RenderTarget(uint32 index=0) const { return _storage->_renderTargets->ObjectAtIndex<Texture>(index); }
		
		bool HasDepthbuffer() const { return _storage->HasDepthbuffer(); }
		bool HasStencilbuffer() const { return _storage->HasStencilbuffer(); }
		
		bool AllowsDepthWrite() const { return _allowDepthWrite; }
		
		float *DepthArray();
		size_t MaxLightsPerTile() const { return _maxLights; }
		
		PostProcessingPipeline *AddPostProcessingPipeline(const std::string& name);
		PostProcessingPipeline *PostProcessingPipelineWithName(const std::string& name);
		void AttachPostProcessingPipeline(PostProcessingPipeline *pipeline);
		void RemovePostProcessingPipeline(PostProcessingPipeline *pipeline);
		
		const std::vector<PostProcessingPipeline *>& PostProcessingPipelines() const { return _PPPipelines; }
		
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
		
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		
		RenderGroups renderGroup;
		
	protected:
		void Initialize();
		
	private:
		Rect _frame;
		Color _clearColor;
		Flags _flags;
		ColorFlags _colorMask;
		GLuint _clearMask;
		float _scaleFactor;
		int32 _priority;
		Vector2 _renderingOffset;
		
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
			
			Plane _absFrustumLeft;
			Plane _absFrustumRight;
			Plane _absFrustumTop;
			Plane _absFrustumBottom;
			Plane _absFrustumFar;
			Plane _absFrustumNear;
		};
		
		Vector2 _lightTiles;
		Vector2 _wantedLightTiles;
		
		bool _allowDepthWrite;
		bool _isStage;
		bool _blend;
		
		Shader *_blitShader;
		
		class Material *_material;
		RenderStorage *_storage;
		Camera *_lodCamera;
		
		bool _useInstancing;
		Texture *_depthTiles;
		Model *_skycube;
		
		float *_depthArray;
		FrameID _depthFrame;
		size_t _depthSize;
		
		size_t _maxLights;
		uint32 _stageCount;
		
		std::vector<PostProcessingPipeline *> _PPPipelines;
		std::map<std::string, PostProcessingPipeline *> _namedPPPipelines;
		
		RNDefineMeta(Camera, SceneNode)
	};
}

#endif /* __RAYNE_CAMERA_H__ */
