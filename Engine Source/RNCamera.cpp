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
	RenderStorage::RenderStorage(BufferFormat format)
	{
		_formatChanged = true;
		_frameChanged  = true;
		_renderTargetsChanged = true;
		
		_boundRenderTargets = 0;
		_renderTargets = new ObjectArray();
		
		_framebuffer = _depthbuffer = _stencilbuffer = 0;
		_scaleFactor = Kernel::SharedInstance()->ScaleFactor();
		_format = (BufferFormat)-1;
		
		SetBufferFormat(format);
		glGenFramebuffers(1, &_framebuffer);
	}
	
	RenderStorage::~RenderStorage()
	{
		if(_depthbuffer)
		{
			glDeleteRenderbuffers(1, &_depthbuffer);
			_stencilbuffer = 0;
			_stencilbuffer = 0;
		}
		
		if(_framebuffer)
			glDeleteFramebuffers(1, &_framebuffer);
		
		_renderTargets->Release();
	}
	
	
	void RenderStorage::SetFrame(const Rect& frame)
	{
		if(_frame != frame)
		{
			_frameChanged = true;
			_frame = frame;
		}
	}
	
	void RenderStorage::SetBufferFormat(BufferFormat format)
	{
		if(_format != format)
		{
			_formatChanged = true;
			_format = format;
			
			_clearMask = GL_COLOR_BUFFER_BIT;
			
			switch(format)
			{
				case BufferFormatColor:
					_clearMask = GL_COLOR_BUFFER_BIT;
					break;
					
				case BufferFormatColorDepth:
					_clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
					break;
					
				case BufferFormatColorDepthStencil:
					_clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
					break;
			}
		}
	}
	
	void RenderStorage::SetRenderTarget(Texture *target, uint32 index)
	{
		target->Bind();
		target->SetLinear(false);
		target->SetGeneratesMipmaps(false);
		target->SetWrappingMode(Texture::WrapModeClamp);
		target->SetFilter(Texture::FilterNearest);
		target->Unbind();
		
		_renderTargets->ReplaceObjectAtIndex(index, target);
		_renderTargetsChanged = true;
	}
	
	void RenderStorage::RemoveRenderTarget(Texture *target)
	{
		_renderTargets->RemoveObject(target);
		_renderTargetsChanged = true;
	}
	
	void RenderStorage::AddRenderTarget(Texture *target)
	{
		if(_renderTargets->Count() >= MaxRenderTargets())
			throw ErrorException(0, 0, 0);
		
		target->Bind();
		target->SetLinear(false);
		target->SetGeneratesMipmaps(false);
		target->SetWrappingMode(Texture::WrapModeClamp);
		target->SetFilter(Texture::FilterNearest);
		target->Unbind();
		
		_renderTargets->AddObject(target);
		_renderTargetsChanged = true;
	}
	
	void RenderStorage::AddRenderTarget(Texture::Format format)
	{
		Texture *target = new Texture(format, Texture::WrapModeClamp, Texture::FilterNearest, false);
		
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
	
	void RenderStorage::CheckFramebufferStatus()
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
	
	void RenderStorage::UpdateDrawBuffers(uint32 count)
	{
#if RN_TARGET_OPENGL
		GLenum buffers[count];
		
		for(uint32 i=0; i<count; i++)
			buffers[i] = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
		
		glDrawBuffers(count, buffers);
#endif
	}
	
	void RenderStorage::UpdateBuffer()
	{
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
			for(machine_uint i=_renderTargets->Count(); i<_boundRenderTargets; i++)
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
				texture->Unbind();
			}
			
#if RN_TARGET_OPENGL
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
#endif
			
#if RN_TARGET_OPENGL_ES
			if(_format == BufferFormatColorDepth)
			{
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, width, height);
				
				RN_CHECKOPENGL();
			}
			
			if(_format == BufferFormatColorDepthStencil)
			{
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, width, height);
				
				RN_CHECKOPENGL();
			}
#endif
			
			CheckFramebufferStatus();
			_frameChanged = false;
		}
	}
	
	uint32 RenderStorage::MaxRenderTargets()
	{
#if GL_MAX_COLOR_ATTACHMENTS
		static GLint maxDrawbuffers = 0;
		
		if(maxDrawbuffers == 0)
			glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxDrawbuffers);
		
		return (uint32)maxDrawbuffers;
#endif
		
		return 1;
	}
	
	
	
	
	Camera::Camera(const Vector2& size) :
		Camera(size, Texture::FormatRGBA8888)
	{}
	
	
	Camera::Camera(const Vector2& size, Texture *target) :
		Camera(size, target, FlagUpdateAspect | FlagFullscreen)
	{}
	
	Camera::Camera(const Vector2& size, Texture *target, Flags flags) :
		Camera(size, target, flags, RenderStorage::BufferFormat::BufferFormatColorDepthStencil)
	{}
	
	
	Camera::Camera(const Vector2& size, Texture::Format targetFormat) :
		Camera(size, targetFormat, FlagUpdateAspect | FlagFullscreen)
	{}
	
	Camera::Camera(const Vector2& size, Texture::Format targetFormat, Flags flags) :
		Camera(size, targetFormat, flags, RenderStorage::BufferFormat::BufferFormatColorDepthStencil)
	{}
	
	
	Camera::Camera(const Vector2& size, Texture *target, Flags flags, RenderStorage::BufferFormat format) :
		_frame(Vector2(0.0f, 0.0f), size),
		_flags(flags),
		RenderingResource("Camera")
	{
		_storage = 0;
		
		RenderStorage *storage = new RenderStorage(format);
		storage->AddRenderTarget(target);
		
		SetRenderStorage(storage);
		Initialize();
		
		World::SharedInstance()->AddCamera(this);
	}
	
	Camera::Camera(const Vector2& size, Texture::Format targetFormat, Flags flags, RenderStorage::BufferFormat format) :
		_frame(Vector2(0.0f, 0.0f), size),
		_flags(flags),
		RenderingResource("Camera")
	{
		_storage = 0;
		
		RenderStorage *storage = new RenderStorage(format);
		storage->AddRenderTarget(targetFormat);
		
		SetRenderStorage(storage);
		Initialize();
		
		World::SharedInstance()->AddCamera(this);
	}
	
	Camera::Camera(const Vector2& size, RenderStorage *storage, Flags flags) :
		_frame(Vector2(0.0f, 0.0f), size),
		_flags(flags),
		RenderingResource("Camera")
	{
		_storage = 0;
		
		Initialize();
		SetRenderStorage(storage);
		
		World::SharedInstance()->AddCamera(this);
	}
	
	Camera::~Camera()
	{
		_storage->Release();
		World::SharedInstance()->RemoveCamera(this);
	}
	
	
	void Camera::Initialize()
	{
		aspect   = 1.0f;
		fov      = 70.0f;
		
		clipnear = 0.1f;
		clipfar  = 500.0f;
		
		_clearColor  = Color(0.193f, 0.435f, 0.753f, 1.0f);
		_scaleFactor = Kernel::SharedInstance()->ScaleFactor();
		
		_material = 0;
		_stage = 0;
		
		Update(0.0f);
		UpdateProjection();
	}
	
	
	
	void Camera::Bind()
	{
		Push();
		Thread *thread = Thread::CurrentThread();
		
		if(thread->CurrentCamera() != this)
			glBindFramebuffer(GL_FRAMEBUFFER, _storage->_framebuffer);
		
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
				glBindFramebuffer(GL_FRAMEBUFFER, other->_storage->_framebuffer);
		}
	}
	
	void Camera::PrepareForRendering()
	{
		_storage->UpdateBuffer();
		
		if(!(_flags & FlagNoClear))
		{
#if RN_PLATFORM_IOS
			glClearDepthf(1.0f);
			glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
#endif
#if RN_PLATFORM_MAC_OS
			glClearDepth(1.0f);
			glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
#endif
			
			glClear(_storage->ClearMask());
		}
		
		glViewport(0, 0, _frame.width * _scaleFactor, _frame.height * _scaleFactor);
	}
	
	
	// Setter
	void Camera::SetFrame(const Rect& frame)
	{
		if(_frame != frame)
		{
			_frame = frame;
			_storage->SetFrame(frame);
			
			UpdateProjection();
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
	
	void Camera::SetRenderStorage(RenderStorage *storage)
	{
		RN_ASSERT(storage, "Render storage mustn't be NULL!");
		
		_storage->Release();
		_storage = storage->Retain<RenderStorage>();
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
				temp->InsertStage(stage);
				return;
			}
			
			temp = temp->_stage;
		}
	}
	
	void Camera::InsertStage(Camera *stage)
	{
		_stage->Release();
		_stage = stage->Retain<Camera>();
		
		UpdateProjection();
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
		
		UpdateProjection();
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
		inverseViewMatrix = Matrix();
		viewMatrix = inverseViewMatrix->Inverse();
		
		if(_flags & FlagFullscreen)
		{
			Rect frame = Kernel::SharedInstance()->Window()->Frame();
			SetFrame(frame);
		}
		
		if(_stage)
		{
			if(_stage->_flags & FlagInherit)
			{
				_stage->SetFrame(_frame);
				
				_stage->SetPosition(Position());
				_stage->SetRotation(Rotation());
			}
			
			_stage->Update(delta);
		}
	}
	
	void Camera::UpdateProjection()
	{
		if(_flags & FlagUpdateAspect)
			aspect = _frame.width / _frame.height;
		
		projectionMatrix->MakeProjectionPerspective(fov, aspect, clipnear, clipfar);
		inverseProjectionMatrix->MakeInverseProjectionPerspective(fov, aspect, clipnear, clipfar);
		
		if(_stage && _stage->_flags & FlagInherit)
		{
			_stage->aspect = aspect;
			_stage->fov    = fov;
			
			_stage->clipfar  = clipfar;
			_stage->clipnear = clipnear;
			
			_stage->UpdateProjection();
		}
	}
}
