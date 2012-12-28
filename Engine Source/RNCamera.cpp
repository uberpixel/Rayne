//
//  RNCamera.cpp
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCamera.h"

namespace RN
{
	Camera *__CurrentCamera = 0;
	
	Camera::Camera(const Vector2& size) :
		_frame(Vector2(0.0f, 0.0f), size),
		_clearColor(0.193f, 0.435f, 0.753f, 1.0f)
	{		
		glGenFramebuffers(1, &_frameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
		
		glGenRenderbuffers(1, &_colorBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, _colorBuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorBuffer);
		
		glGenRenderbuffers(1, &_depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, _depthBuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBuffer);

		glGenRenderbuffers(1, &_stencilBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, _stencilBuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _stencilBuffer);
		
		arc = 70.0f;
		aspect = 1.0f;
		clipnear = 0.1f;
		clipfar = 500.0f;
		
		_bound = false;
		SetFrame(_frame);
		
		UpdateProjection();
		UpdateCamera();
	}
	
	Camera::~Camera()
	{
		glDeleteFramebuffers(1, &_frameBuffer);
		
		glDeleteRenderbuffers(1, &_colorBuffer);
		glDeleteRenderbuffers(1, &_depthBuffer);
		glDeleteRenderbuffers(1, &_stencilBuffer);
		
		if(_bound)
			Unbind();
	}
	
	
	void Camera::Bind()
	{
		if(!_bound)
		{
			_bound = true;
			_previous = __CurrentCamera;
			__CurrentCamera = this;
			
			glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
		}
	}
	
	void Camera::Unbind()
	{
		RN::Assert(__CurrentCamera == this);
		
		_bound = false;
		__CurrentCamera = _previous;
		
		if(_previous)
			glBindFramebuffer(GL_FRAMEBUFFER, _previous->_frameBuffer);
	}
	
	void Camera::PrepareForRendering()
	{
		RN::Assert(__CurrentCamera == this);
		
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
