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
	Camera::Camera(const Vector2& size, Flags _flags) :
		_frame(Vector2(0.0f, 0.0f), size),
		_clearColor(0.193f, 0.435f, 0.753f, 1.0f),
		RenderingResource("Camera")
	{
		_material = 0;
		_stage    = 0;
		_surfaceMaterial = 0;
		
		_isLinear = false;
		_texture  = new Texture(Texture::FormatRGBA8888, Texture::WrapModeClamp, Texture::FilterNearest, _isLinear);
		_texture->SetGeneratesMipmaps(false);
		
		_scaleFactor = Kernel::SharedInstance()->ScaleFactor();
		flags        = _flags;
		
		glGenFramebuffers(1, &_framebuffer);
		
		SetDefaultValues();
		Bind();
		
		_depthbuffer   = 0;
		_stencilbuffer = 0;
		
		glGenRenderbuffers(1, &_depthbuffer);
		
		_stencilbuffer = _depthbuffer;
		_packedStencil = true;
		
		glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthbuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _stencilbuffer);
		
		if(flags & FlagFullscreen)
			_frame = Kernel::SharedInstance()->Window()->Frame();
			
		try
		{
			SetFrame(_frame);
		}
		catch(ErrorException e)
		{
			Unbind();
			throw e;
		}
		
		Unbind();
		UpdateCamera();
		
		World::SharedInstance()->AddCamera(this);
	}
	
	Camera::~Camera()
	{
		glDeleteFramebuffers(1, &_framebuffer);
		
		if(_depthbuffer)
			glDeleteRenderbuffers(1, &_depthbuffer);
		
		if(_stencilbuffer && !_packedStencil)
			glDeleteRenderbuffers(1, &_stencilbuffer);
		
		if(_texture)
			_texture->Release();
		
		_stage->Release();
		World::SharedInstance()->RemoveCamera(this);
	}
	
	void Camera::SetDefaultValues()
	{
		aspect   = 1.0f;
		arc      = 70.0f;
		clipnear = 0.1f;
		clipfar  = 500.0f;
	}
	
	void Camera::CheckError()
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
		
		glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
		glClear(clearMask);
		
		glViewport(0, 0, _frame.width * _scaleFactor, _frame.height * _scaleFactor);
	}
		
	
	
	void Camera::SetFrame(const Rect& frame)
	{
		_frame = frame;
		
		Bind();
		
		uint32 width  = (uint32)(_frame.width * _scaleFactor);
		uint32 height = (uint32)(_frame.height * _scaleFactor);
		
		if(_texture)
		{
			_texture->Bind();
			
#if GL_SRGB8_ALPHA8
			if(_isLinear)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
				RN_CHECKOPENGL();
			}
			else
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
				RN_CHECKOPENGL();
			}
#else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
			RN_CHECKOPENGL();
#endif
			
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture->Name(), 0);
			RN_CHECKOPENGL();		
			
			_texture->_width  = width;
			_texture->_height = height;
			_texture->Unbind();
		}
		
		if(_packedStencil)
		{
			if(_depthbuffer)
			{
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
#endif
			
#if RN_PLATFORM_IOS
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, width, height);
#endif
				
				RN_CHECKOPENGL();
			}
		}
		else
		{
			if(_depthbuffer)
			{
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
#endif
				
#if RN_PLATFORM_IOS
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, width, height);
#endif
				
				RN_CHECKOPENGL();
			}
			
			if(_stencilbuffer)
			{
				glBindRenderbuffer(GL_RENDERBUFFER, _stencilbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height);
				
				RN_CHECKOPENGL();
			}
		}
		
		glFlush();
		
		CheckError();
		UpdateProjection();
		Unbind();
		UpdateStage(true);
	}
	
	void Camera::SetClearColor(const Color& color)
	{
		_clearColor = color;
		UpdateStage(true);
	}
	
	void Camera::SetMaterial(class Material *material)
	{
		_material->Release();
		_material = material->Retain<class Material>();
	}
	
	void Camera::SetSurfaceMaterial(class Material *material)
	{
		_surfaceMaterial->Release();
		_surfaceMaterial = material->Retain<class Material>();
	}
	
	
	void Camera::AddStage(Camera *stage)
	{
		if(_stage)
		{
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
		else
		{
			InsertStage(stage);
		}
	}
	
	void Camera::InsertStage(Camera *stage)
	{
		stage->_stage->Release();
		stage->_stage = _stage->Retain<Camera>();
		
		_stage = stage;
		UpdateStage(true);
		
		World::SharedInstance()->RemoveCamera(stage);
	}
	
	void Camera::RemoveStage(Camera *stage)
	{
	}
	
	void Camera::UpdateStage(bool updateFrame) const
	{
		if(_stage && _stage->flags & FlagInherit)
		{
			_stage->arc = arc;
			_stage->aspect = aspect;
			_stage->clipnear = clipnear;
			_stage->clipfar = clipfar;
			
			if(updateFrame)
				_stage->SetFrame(_frame);
			
			_stage->SetPosition(Position());
			_stage->SetRotation(Rotation());
			
			_stage->SetClearColor(_clearColor);
			_stage->UpdateCamera();
		}
	}
	
	void Camera::UpdateProjection()
	{
		if(flags & FlagUpdateAspect)
			aspect = _frame.width / _frame.height;
		
		_projectionMatrix.MakeProjectionPerspective(arc, aspect, clipnear, clipfar);
		_inverseProjectionMatrix.MakeInverseProjectionPerspective(arc, aspect, clipnear, clipfar);
		
		UpdateStage(true);
	}
	
	void Camera::UpdateCamera()
	{
		_inverseViewMatrix = Matrix();
		_viewMatrix = _inverseViewMatrix.Inverse();
		
		if(flags & FlagFullscreen)
		{
			Rect frame = Kernel::SharedInstance()->Window()->Frame();
			if(frame != _frame)
			{
				SetFrame(frame);
			}
			else
			{
				UpdateStage(false);
			}
		}
		else
		{
			UpdateStage(false);
		}
	}
}
