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
#include "RNArray.h"
#include "RNRenderingResource.h"
#include "RNTexture.h"
#include "RNRect.h"
#include "RNTransform.h"
#include "RNColor.h"

namespace RN
{
	class Texture;
	class Material;
	
	class Camera : public Object, public Transform, public RenderingResource
	{
	public:
		enum
		{
			FlagDrawTarget = (1 << 0),
			FlagUpdateAspect = (1 << 1),
			FlagInherit = (1 << 2),
			FlagFullscreen = (1 << 3),
			
		};
		typedef uint32 Flags;
		
		typedef enum
		{
			BufferFormatColor,
			BufferFormatColorDepth,
			BufferFormatColorDepthStencil
		} BufferFormat;
		
		Camera(const Vector2& size);
		
		Camera(const Vector2& size, Texture *target);
		Camera(const Vector2& size, Texture *target, Flags flags);
		Camera(const Vector2& size, Texture *target, Flags flags, BufferFormat format);
		
		Camera(const Vector2& size, Texture::Format targetFormat);
		Camera(const Vector2& size, Texture::Format targetFormat, Flags flags);
		Camera(const Vector2& size, Texture::Format targetFormat, Flags flags, BufferFormat format);
		
		virtual ~Camera();
		
		void Bind();
		void Unbind();
		void PrepareForRendering();
		
		void SetFrame(const Rect& frame);
		void SetClearColor(const Color& color);
		void SetMaterial(Material *material);
		void SetBufferFormat(BufferFormat format);
		
		void SetRenderTarget(Texture *target, uint32 index);
		void AddRenderTarget(Texture *target);
		void AddRenderTarget(Texture::Format format);
		void RemoveRenderTarget(Texture *target);
		
		void AddStage(Camera *stage);
		void InsertStage(Camera *stage);
		void ReplaceStage(Camera *stage);
		void RemoveStage(Camera *stage);
		
		void Update(float delta);
		void UpdateProjection();
		
		bool HasDepthbuffer() const { return (_format == BufferFormatColorDepth || _format == BufferFormatColorDepthStencil); }
		bool HasStencilbuffer() const { return _format == BufferFormatColorDepthStencil; }
		
		GLuint Framebuffer() const { return _framebuffer; }
		const Color& ClearColor() const { return _clearColor; }
		const Rect& Frame() const { return _frame; }
		Material *Material() const { return _material; }
		Camera *Stage() const { return _stage; }
		Flags CameraFlags() const { return _flags; }
		
		uint32 RenderTargets() const { return (uint32)_renderTargets->Count(); }
		Texture *RenderTarget(uint32 index=0) const { return (Texture *)_renderTargets->ObjectAtIndex(index); }
		
		const class Matrix& ProjectionMatrix() const { return _projectionMatrix; }
		const class Matrix& InverseProjectionMatrix() const { return _inverseProjectionMatrix; }
		
		const class Matrix& ViewMatrix() const { return _viewMatrix; }
		const class Matrix& InverseViewMatrix() const { return _inverseViewMatrix; }
		
		static uint32 MaxRenderTargets();
		
		float fov;
		float aspect;
		float clipnear;
		float clipfar;
		
	protected:
		void Initialize();
		void CheckFramebufferStatus();
		void UpdateBuffer();
		
		class Matrix _projectionMatrix;
		class Matrix _inverseProjectionMatrix;
		
		class Matrix _viewMatrix;
		class Matrix _inverseViewMatrix;
		
	private:
		void UpdateDrawBuffers(uint32 count);
		
		GLuint _framebuffer;
		GLuint _depthbuffer;
		GLuint _stencilbuffer;
		
		Rect _frame;
		Color _clearColor;
		BufferFormat _format;
		Flags _flags;
		float _scaleFactor;
		bool _frameChanged;
		bool _formatChanged;
		bool _renderTargetsChanged;
		uint32 _boundRenderTargets;
		
		class Material *_material;
		ObjectArray *_renderTargets;
		Camera *_stage;
	};
}

#endif /* __RAYNE_CAMERA_H__ */
