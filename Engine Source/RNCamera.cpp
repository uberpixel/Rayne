//
//  RNCamera.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCamera.h"
#include "RNThread.h"

namespace RN
{
	Camera::Camera(const Vector2& size) :
		_frame(Vector2(0.0f, 0.0f), size),
		_clearColor(0.193f, 0.435f, 0.753f, 1.0f)
	{
		_ownsBuffer = true;
		
		glGenFramebuffers(1, &_frameBuffer);
		Bind();
		
		glGenRenderbuffers(1, &_colorBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, _colorBuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorBuffer);
		
		glGenRenderbuffers(1, &_depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, _depthBuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBuffer);

		glGenRenderbuffers(1, &_stencilBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, _stencilBuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _stencilBuffer);
		
		try
		{
			CheckError();
		}
		catch(ErrorException e)
		{
			Unbind();
			throw e;
		}
		
		Unbind();
		
		arc = 70.0f;
		aspect = 1.0f;
		clipnear = 0.1f;
		clipfar = 500.0f;
		
		SetFrame(_frame);
		
		UpdateProjection();
		UpdateCamera();
	}
	
	Camera::Camera(GLuint framebuffer) :
		_frame(Vector2(0.0f, 0.0f), Vector2(1024.0f, 768.0f)),
		_clearColor(0.193f, 0.435f, 0.753f, 1.0f)
	{
		_ownsBuffer = false;
		_frameBuffer = framebuffer;
		
		Bind();
		
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, (GLint *)&_colorBuffer);
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, (GLint *)&_depthBuffer);
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, (GLint *)&_stencilBuffer);
		
		try
		{
			CheckError();
		}
		catch(ErrorException e)
		{
			Unbind();
			throw e;
		}
		
		Unbind();
		
		arc = 70.0f;
		aspect = 1.0f;
		clipnear = 0.1f;
		clipfar = 500.0f;
		
		SetFrame(_frame);
		
		UpdateProjection();
		UpdateCamera();
	}
	
	Camera::~Camera()
	{
		if(_ownsBuffer)
		{
			glDeleteFramebuffers(1, &_frameBuffer);
			
			glDeleteRenderbuffers(1, &_colorBuffer);
			glDeleteRenderbuffers(1, &_depthBuffer);
			glDeleteRenderbuffers(1, &_stencilBuffer);
		}
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
					throw ErrorException(kErrorGroupGraphics, kGraphicsGroupOpenGL, kGraphicsFramebufferUndefined);
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
			thread->PopCamera();
			
			Camera *other = thread->CurrentCamera();
			if(other && other != this)
				glBindFramebuffer(GL_FRAMEBUFFER, other->_frameBuffer);
		}
	}
	
	void Camera::PrepareForRendering()
	{
		glViewport(_frame.x, _frame.y, _frame.width, _frame.height);
		
		glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
		glClearStencil(0);
		glClearDepth(1.0f);
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
		
	
	
	
	void Camera::SetFrame(const Rect& frame)
	{
		_frame = frame;
		
		Bind();
		
		uint32 width  = (uint32)_frame.width;
		uint32 height = (uint32)_frame.height;
		
		glBindRenderbuffer(GL_RENDERBUFFER, _depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		
		glBindRenderbuffer(GL_RENDERBUFFER, _stencilBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height);
		
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
