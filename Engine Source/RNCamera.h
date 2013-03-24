//
//  RNCamera.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CAMERA_H__
#define __RAYNE_CAMERA_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNTransform.h"
#include "RNRenderStorage.h"
#include "RNTexture.h"
#include "RNMaterial.h"
#include "RNRect.h"
#include "RNPlane.h"
#include "RNColor.h"
#include "RNModel.h"

namespace RN
{
	class Camera : public Transform
	{
	public:
		enum
		{
			FlagDrawTarget = (1 << 0),
			FlagUpdateAspect = (1 << 1),
			FlagFullscreen = (1 << 2),
			FlagNoClear = (1 << 3),
			FlagInheritPosition = (1 << 4),
			FlagInheritFrame = (1 << 5),
			FlagInheritProjection = (1 << 6),
			FlagUpdateStorageFrame = (1 << 7),
			
			
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
		
		Camera(const Vector2& size);
		
		Camera(const Vector2& size, Texture *target);
		Camera(const Vector2& size, Texture *target, Flags flags);
		Camera(const Vector2& size, Texture *target, Flags flags, RenderStorage::BufferFormat format);
		
		Camera(const Vector2& size, Texture::Format targetFormat);
		Camera(const Vector2& size, Texture::Format targetFormat, Flags flags);
		Camera(const Vector2& size, Texture::Format targetFormat, Flags flags, RenderStorage::BufferFormat format);
		
		Camera(const Vector2& size, RenderStorage *storage, Flags flags);
		
		virtual ~Camera();
		
		void Bind();
		void Unbind();
		void PrepareForRendering();
		
		void SetFrame(const Rect& frame);
		void SetClearColor(const Color& color);
		void SetMaterial(Material *material);
		void SetRenderStorage(RenderStorage *storage);
		void SetClearMask(ClearFlags mask);
		void SetColorMask(ColorFlags mask);
		void SetAllowsDepthWrite(bool flag);
		void SetSkyCube(Model *skycube);
		
		void AddStage(Camera *stage);
		void InsertStage(Camera *stage);
		void ReplaceStage(Camera *stage);
		void RemoveStage(Camera *stage);
		
		void ActivateTiledLightLists(Texture *depthtiles){ _depthTiles = depthtiles; }
		Texture *DepthTiles() { return _depthTiles; }
		
		void Update(float delta);
		void PostUpdate();
		void UpdateProjection();
		
		Vector3 ToWorld(const Vector3& dir);
		void UpdateFrustum();
		bool InFrustum(const Vector3& position, float radius);
		
		const Vector3& FrustumCenter() const {return _frustumCenter;}
		float const FrustumRadius() const {return _frustumRadius;}
		
		RenderStorage *Storage() const { return _storage; }
		const Color& ClearColor() const { return _clearColor; }
		const Rect& Frame() const { return _frame; }
		Material *Material() const { return _material; }
		Camera *Stage() const { return _stage; }
		Flags CameraFlags() const { return _flags; }
		
		Vector2 LightTiles() const { return _lightTiles; }
		Model *SkyCube() const { return _skycube; }
		
		uint32 RenderTargets() const { return (uint32)_storage->_renderTargets->Count(); }
		Texture *RenderTarget(uint32 index=0) const { return _storage->_renderTargets->ObjectAtIndex(index); }
		
		bool HasDepthbuffer() const { return _storage->HasDepthbuffer(); }
		bool HasStencilbuffer() const { return _storage->HasStencilbuffer(); }
		
		bool AllowsDepthWrite() const { return _allowDepthWrite; }
		
		float fov;
		float aspect;
		float clipnear;
		float clipfar;
		
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		
	protected:
		void Initialize();
		
	private:
		Rect _frame;
		Color _clearColor;
		Flags _flags;
		ColorFlags _colorMask;
		GLuint _clearMask;
		float _scaleFactor;
		
		Vector3 _frustumCenter;
		float _frustumRadius;
		Plane _frustumLeft;
		Plane _frustumRight;
		Plane _frustumTop;
		Plane _frustumBottom;
		
		Vector2 _lightTiles;
		
		bool _allowDepthWrite;
		
		class Material *_material;
		RenderStorage *_storage;
		Camera *_stage;
		
		Texture *_depthTiles;
		Model *_skycube;
		
		RNDefineConstructorlessMeta(Camera, Transform)
	};
}

#endif /* __RAYNE_CAMERA_H__ */
