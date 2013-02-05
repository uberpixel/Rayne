//
//  RNCamera.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCamera.h"
#include "RNThread.h"
#include "RNKernel.h"
#include "RNTexture.h"
#include "RNMaterial.h"
#include "RNWorld.h"

namespace RN
{
	Camera::Camera(const Vector2& size) :
		Camera(size, Texture::FormatRGBA8888)
	{}
	
	
	Camera::Camera(const Vector2& size, Texture *target) :
		Camera(size, target, FlagUpdateAspect | FlagFullscreen)
	{}
	
	Camera::Camera(const Vector2& size, Texture *target, Flags flags) :
		Camera(size, target, flags, BufferFormatColorDepthStencil)
	{}
	
	
	Camera::Camera(const Vector2& size, Texture::Format targetFormat) :
		Camera(size, targetFormat, FlagUpdateAspect | FlagFullscreen)
	{}
	
	Camera::Camera(const Vector2& size, Texture::Format targetFormat, Flags flags) :
		Camera(size, targetFormat, flags, BufferFormatColorDepthStencil)
	{}
	
	
	Camera::Camera(const Vector2& size, Texture *target, Flags flags, BufferFormat format) :
		_frame(Vector2(0.0f, 0.0f), size),
		_flags(flags),
		_format(format),
		RenderingResource("Camera")
	{
		Initialize();
		AddRenderTarget(target);
		
		World::SharedInstance()->AddCamera(this);
	}
	
	Camera::Camera(const Vector2& size, Texture::Format targetFormat, Flags flags, BufferFormat format) :
		_frame(Vector2(0.0f, 0.0f), size),
		_flags(flags),
		_format(format),
		RenderingResource("Camera")
	{
		Initialize();
		AddRenderTarget(targetFormat);
		
		World::SharedInstance()->AddCamera(this);
	}
	
	Camera::~Camera()
	{
		_renderTargets->Release();
		
		if(_depthbuffer)
		{
			glDeleteRenderbuffers(1, &_depthbuffer);
			_stencilbuffer = 0;
			_stencilbuffer = 0;
		}
		
		if(_framebuffer)
			glDeleteFramebuffers(1, &_framebuffer);
		
		World::SharedInstance()->RemoveCamera(this);
	}
	
	
	void Camera::Initialize()
	{
		aspect   = 1.0f;
		fov      = 70.0f;
		
		clipnear = 0.1f;
		clipfar  = 500.0f;
		
		_framebuffer = _depthbuffer = _stencilbuffer = 0;
		_clearColor  = Color(0.193f, 0.435f, 0.753f, 1.0f);
		
		_formatChanged = true;
		_frameChanged  = true;
		_renderTargetsChanged = true;
		_boundRenderTargets = 0;
		
		_material = 0;
		_renderTargets = new ObjectArray();
		_stage = 0;
		
		Update(0.0f);
		UpdateProjection();
		
		glGenFramebuffers(1, &_framebuffer);
		_scaleFactor = Kernel::SharedInstance()->ScaleFactor();
	}
	
	
	void Camera::Bind()
	{
		Push();
		Thread *thread = Thread::CurrentThread();
		
		if(thread->CurrentCamera() != this)
			glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
		
		thread->PushCamera(this);
	}
	
	void Camera::Unbind()
	{
		Thread *thread = Thread::CurrentThread();
		if(thread->CurrentCamera() == this)
		{
			Pop();
			thread->PopCamera();
			
			Camera *other = thread->CurrentCamera();
			if(other && other != this)
				glBindFramebuffer(GL_FRAMEBUFFER, other->_framebuffer);
		}
	}
	
	void Camera::PrepareForRendering()
	{
		GLenum clearMask = GL_COLOR_BUFFER_BIT;
		if(_depthbuffer)
		{
#if RN_PLATFORM_IOS
			glClearDepthf(1.0f);
#endif
#if RN_PLATFORM_MAC_OS
			glClearDepth(1.0f);
#endif
			clearMask |= GL_DEPTH_BUFFER_BIT;
		}
		
		if(_stencilbuffer)
		{
			glClearStencil(0);
			clearMask |= GL_STENCIL_BUFFER_BIT;
		}
		
		UpdateBuffer();
		
		glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
		glClear(clearMask);
		
		glViewport(0, 0, _frame.width * _scaleFactor, _frame.height * _scaleFactor);
	}
	
	
	// Setter
	void Camera::SetFrame(const Rect& frame)
	{
		if(_frame != frame)
		{
			_frame = frame;
			_frameChanged = true;
		}
	}
	
	void Camera::SetClearColor(const Color& clearColor)
	{
		_clearColor = clearColor;
	}
	
	void Camera::SetMaterial(class Material *material)
	{
		_material->Release();
		_material = material->Retain<class Material>();
	}
	
	void Camera::SetBufferFormat(BufferFormat format)
	{
		if(_format != format)
		{
			_format = format;
			_formatChanged = true;
		}
	}
	
	// Render target handling
	void Camera::SetRenderTarget(Texture *target, uint32 index)
	{
		_renderTargets->ReplaceObjectAtIndex(index, target);
		_renderTargetsChanged = true;
	}
	
	void Camera::RemoveRenderTarget(Texture *target)
	{
		_renderTargets->RemoveObject(target);
		_renderTargetsChanged = true;
	}
	
	void Camera::AddRenderTarget(Texture *target)
	{
		if(_renderTargets->Count() >= MaxRenderTargets())
			throw ErrorException(0, 0, 0);
		
		_renderTargets->AddObject(target);
		_renderTargetsChanged = true;
	}
	
	void Camera::AddRenderTarget(Texture::Format format)
	{
		Texture *target = new Texture(format, Texture::WrapModeClamp, Texture::FilterNearest, true);
		try
		{
			AddRenderTarget(target);
			target->Release();
		}
		catch(ErrorException e)
		{
			target->Release();
		}
	}
	
	// Stages
	void Camera::AddStage(Camera *stage)
	{
		if(!_stage)
		{
			InsertStage(stage);
			return;
		}
		
		Camera *temp = _stage;
		while(temp)
		{
			if(!temp->_stage)
			{
				temp->InsertStage(_stage);
				return;
			}
			
			temp = temp->_stage;
		}
	}
	
	void Camera::InsertStage(Camera *stage)
	{
		_stage->Release();
		_stage = stage->Retain<Camera>();
		
		World::SharedInstance()->RemoveCamera(_stage);
	}
	
	void Camera::ReplaceStage(Camera *stage)
	{
		Camera *temp = _stage ? _stage->_stage->Retain<Camera>() : 0;
		
		_stage->Release();
		_stage = stage->Retain<Camera>();
		
		if(_stage->_stage)
			_stage->RemoveStage(_stage->_stage);
		
		_stage->_stage = temp;
		World::SharedInstance()->RemoveCamera(_stage);
	}
	
	void Camera::RemoveStage(Camera *stage)
	{
		if(_stage == stage)
		{
			World::SharedInstance()->AddCamera(_stage);
			stage = stage->_stage->Retain<Camera>();
			
			_stage->Release();
			_stage = stage;
			
			return;
		}
		
		Camera *temp = _stage;
		while(temp)
		{
			if(temp->_stage == stage)
			{
				World::SharedInstance()->AddCamera(temp->_stage);
				stage = stage->_stage->Retain<Camera>();
				
				temp->_stage->Release();
				temp->_stage = stage;
				
				return;
			}
			
			temp = temp->_stage;
		}
	}
	
	// Helper
	void Camera::Update(float delta)
	{
		_inverseViewMatrix = Matrix();
		_viewMatrix = _inverseViewMatrix.Inverse();
		
		//_viewMatrix = Matrix();
		//_inverseViewMatrix = _viewMatrix.Inverse();
		
		if(_flags & FlagFullscreen)
		{
			Rect frame = Kernel::SharedInstance()->Window()->Frame();
			SetFrame(frame);
		}
		
		if(_stage && _stage->_flags & FlagInherit)
		{
			_stage->SetFrame(_frame);
			_stage->aspect = aspect;
			_stage->fov    = fov;
			
			_stage->clipfar  = clipfar;
			_stage->clipnear = clipnear;
			
			_stage->SetPosition(Position());
			_stage->SetRotation(Rotation());
		}
	}
	
	void Camera::UpdateProjection()
	{
		if(_flags & FlagUpdateAspect)
			aspect = _frame.width / _frame.height;
		
		_projectionMatrix.MakeProjectionPerspective(fov, aspect, clipnear, clipfar);
		_inverseProjectionMatrix.MakeInverseProjectionPerspective(fov, aspect, clipnear, clipfar);
	}
	
	void Camera::CheckFramebufferStatus()
	{
#if RN_TARGET_OPENGL
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		
		if(status != GL_FRAMEBUFFER_COMPLETE)
		{
			switch(status)
			{
				case GL_FRAMEBUFFER_UNDEFINED:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferUndefined);
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteAttachment);
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteMissingAttachment);
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteDrawBuffer);
					break;
					
				case GL_FRAMEBUFFER_UNSUPPORTED:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferUnsupported);
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteMultisample);
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteLayerTargets);
					break;
					
				default:
					printf("Unknown framebuffer status %i\n", status);
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferGenericError);
					break;
			}
		}
#endif
		
#if RN_TARGET_OPENGL_ES
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if(status != GL_FRAMEBUFFER_COMPLETE)
		{
			switch(status)
			{
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteAttachment);
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteDimensions);
					break;
					
				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferIncompleteMissingAttachment);
					break;
					
				case GL_FRAMEBUFFER_UNSUPPORTED:
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferUnsupported);
					break;
					
				default:
					printf("Unknown framebuffer status %i\n", status);
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferGenericError);
					break;
			}
		}
#endif
	}
	
	void Camera::UpdateDrawBuffers(uint32 count)
	{
		GLenum buffers[count];
		
		for(uint32 i=0; i<count; i++)
			buffers[i] = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
		
		glDrawBuffers(count, buffers);
	}
	
	void Camera::UpdateBuffer()
	{
		Bind();
		
		if(_formatChanged)
		{
			// Remove unused buffers
			if(_stencilbuffer && _format != BufferFormatColorDepthStencil)
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
				_stencilbuffer = 0;
			}
			
			if(_depthbuffer && (_format != BufferFormatColorDepth || _format != BufferFormatColorDepthStencil))
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
				glDeleteRenderbuffers(1, &_depthbuffer);
				_depthbuffer = 0;
			}
			
			// Create new buffers
			if(_depthbuffer == 0 && (_format == BufferFormatColorDepth || _format == BufferFormatColorDepthStencil))
			{
				glGenRenderbuffers(1, &_depthbuffer);
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthbuffer);
				
				if(_format == BufferFormatColorDepthStencil)
				{
					_stencilbuffer = _depthbuffer;
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _stencilbuffer);
				}
			}
		
			_formatChanged = false;
			_frameChanged  = true;
		}
		
		if(_renderTargetsChanged)
		{
			for(uint32 i=0; i<_boundRenderTargets; i++)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, (GLenum)(GL_COLOR_ATTACHMENT0 + i), GL_TEXTURE_2D, 0, 0);
			}
			
			for(machine_uint i=0; i<_renderTargets->Count(); i++)
			{
				Texture *texture = (Texture *)_renderTargets->ObjectAtIndex(i);
				glFramebufferTexture2D(GL_FRAMEBUFFER, (GLenum)(GL_COLOR_ATTACHMENT0 + i), GL_TEXTURE_2D, texture->Name(), 0);
			}
			
			_boundRenderTargets = (uint32)_renderTargets->Count();
			UpdateDrawBuffers(_boundRenderTargets);
			
			_renderTargetsChanged = false;
		}
		
		// Allocate storage for the buffers
		if(_frameChanged)
		{
			uint32 width = (uint32)(_frame.width * _scaleFactor);
			uint32 height = (uint32)(_frame.height * _scaleFactor);
			
			for(machine_uint i=0; i<_renderTargets->Count(); i++)
			{
				Texture *texture = (Texture *)_renderTargets->ObjectAtIndex(i);
				
				texture->Bind();
				texture->SetData(0, width, height, Texture::FormatRGBA8888);
				
				glFramebufferTexture2D(GL_FRAMEBUFFER, (GLenum)(GL_COLOR_ATTACHMENT0 + i), GL_TEXTURE_2D, texture->Name(), 0);
				RN_CHECKOPENGL();
				
				texture->Unbind();
			}
			
			if(_format == BufferFormatColorDepth)
			{
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
			}
			
			if(_format == BufferFormatColorDepthStencil)
			{
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
			}
			
			try
			{
				CheckFramebufferStatus();
				_frameChanged = false;
			}
			catch(ErrorException e)
			{
				Unbind();
				throw e;
			}
		}
		
		Unbind();
	}
	
	uint32 Camera::MaxRenderTargets()
	{
#if GL_MAX_DRAW_BUFFERS
		static GLint maxDrawbuffers = 0;
		
		if(maxDrawbuffers == 0)
			glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxDrawbuffers);
		
		return (uint32)maxDrawbuffers;
#endif
		
		return 1;
	}
}
