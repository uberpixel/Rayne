//
//  RNCamera.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCamera.h"
#include "RNThread.h"
#include "RNKernel.h"

namespace RN
{
	Camera::Camera(const Vector2& size) :
		_frame(Vector2(0.0f, 0.0f), size),
		_clearColor(0.193f, 0.435f, 0.753f, 1.0f),
		RenderingResource("Camera (RTT)")
	{
		_ownsBuffer = true;
		
		_current    = 0;
		_texture[0] = new Texture(Texture::FormatRGBA8888, Texture::WrapModeClamp);
		_texture[1] = 0;
		
		glGenFramebuffers(1, &_frameBuffer);
		Bind();
		
		_depthBuffer   = 0;
		_stencilBuffer = 0;
		
		glGenRenderbuffers(1, &_depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, _depthBuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBuffer);

		/*glGenRenderbuffers(1, &_stencilBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, _stencilBuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _stencilBuffer);*/
		
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
		
		SetDefaultValues();
		UpdateProjection();
		UpdateCamera();
	}
	
	Camera::Camera(GLuint framebuffer, const Vector2& size) :
		_frame(Vector2(0.0f, 0.0f), size),
		_clearColor(0.193f, 0.435f, 0.753f, 1.0f),
		RenderingResource("Camera")
	{
		_ownsBuffer = false;
		_frameBuffer = framebuffer;
		
		_current    = 0;
		_texture[0] = 0;
		_texture[1] = 0;
		
		Bind();
		
		_depthBuffer = 0;
		_stencilBuffer = 0;
		
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, (GLint *)&_depthBuffer);
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, (GLint *)&_stencilBuffer);
		
		Unbind();
		
		SetDefaultValues();
		UpdateProjection();
		UpdateCamera();
	}
	
	Camera::~Camera()
	{
		if(_ownsBuffer)
		{
			glDeleteFramebuffers(1, &_frameBuffer);
			
			if(_depthBuffer)
				glDeleteRenderbuffers(1, &_depthBuffer);
			
			if(_stencilBuffer)
				glDeleteRenderbuffers(1, &_stencilBuffer);
		}
		
		if(_texture[0])
			_texture[0]->Release();
		
		if(_texture[1])
			_texture[1]->Release();
	}
	
	
	void Camera::MakeDoubleBuffered()
	{
		if(_texture[1] == 0)
		{
			_texture[1] = new Texture(Texture::FormatRGBA8888, Texture::WrapModeClamp);
		}
	}
	
	void Camera::SwitchBuffers()
	{
		_current = (_current == 0) ? 1 : 0;
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture[_current]->Name(), 0);
	}
	
	
	void Camera::SetDefaultValues()
	{
		arc = 70.0f;
		aspect = 1.0f;
		clipnear = 0.1f;
		clipfar = 500.0f;
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
			glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
		
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
				glBindFramebuffer(GL_FRAMEBUFFER, other->_frameBuffer);
		}
	}
	
	void Camera::PrepareForRendering()
	{
		glViewport(_frame.x, _frame.y, _frame.width, _frame.height);
		
		
		GLenum clearMask = GL_COLOR_BUFFER_BIT;
		if(_depthBuffer)
		{
#if RN_PLATFORM_IOS
			glClearDepthf(1.0f);
#endif
#if RN_PLATFORM_MAC_OS
			glClearDepth(1.0f);
#endif
			clearMask |= GL_DEPTH_BUFFER_BIT;
		}
		
		if(_stencilBuffer)
		{
			glClearStencil(0);
			clearMask |= GL_STENCIL_BUFFER_BIT;
		}
		
		glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
		glClear(clearMask);
	}
		
	
	
	
	void Camera::SetFrame(const Rect& frame)
	{
		_frame = frame;
		
		Bind();
		
		uint32 width  = (uint32)_frame.width;
		uint32 height = (uint32)_frame.height;
		
		for(int i=0; i<2; i++)
		{
			if(_texture[i])
			{
				_texture[i]->Bind();
				
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture[i]->Name(), 0);
				
				Kernel::CheckOpenGLError("glTexImage2D");
				
				_texture[i]->Unbind();
			}
		}
		
		if(_depthBuffer)
		{
#if RN_PLATFORM_MAC_OS
			glBindRenderbuffer(GL_RENDERBUFFER, _depthBuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
#endif
			
#if RN_PLATFORM_IOS
			glBindRenderbuffer(GL_RENDERBUFFER, _depthBuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, width, height);
#endif
			
			Kernel::CheckOpenGLError("glRenderBufferStorage");
		}
		
		if(_stencilBuffer)
		{
			glBindRenderbuffer(GL_RENDERBUFFER, _stencilBuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height);
			
			Kernel::CheckOpenGLError("glRenderBufferStorage");
		}
		
		CheckError();
		Unbind();
	}
	
	void Camera::SetClearColor(const Color& color)
	{
		_clearColor = color;
	}
	
	
	void Camera::UpdateProjection()
	{
		_projectionMatrix.MakeProjectionPerspective(arc, aspect, clipnear, clipfar);
		_inverseProjectionMatrix.MakeInverveProjectionPerspective(arc, aspect, clipnear, clipfar);
	}
	
	void Camera::UpdateCamera()
	{
		_viewMatrix = rotation.RotationMatrix();
		_viewMatrix.Transpose();
		_viewMatrix.Translate(position * (-1));
		
		_inverseViewMatrix.MakeTranslate(position);
		_inverseViewMatrix.Rotate(rotation);
	}
}
