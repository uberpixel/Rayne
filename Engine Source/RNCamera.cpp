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
			
			_clearMask = 0;
			
			RN_ASSERT(format > 0, "You must specify at least one buffer in the buffer format!");
			
			if(_format & BufferFormatStencil)
			{
				RN_ASSERT((_format & BufferFormatDepth), "When specifying a stencil buffer, you must also specify a depth buffer!");
			}
			
			if(_format & BufferFormatColor)
				_clearMask |= GL_COLOR_BUFFER_BIT;
			
			if(_format & BufferFormatDepth)
				_clearMask |= GL_DEPTH_BUFFER_BIT;
			
			if(_format & BufferFormatStencil)
				_clearMask |= GL_STENCIL_BUFFER_BIT;
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
			if(_boundRenderTargets > 0 && !(_format & BufferFormatColor))
			{
				for(machine_uint i=0; i<_boundRenderTargets; i++)
				{
					glFramebufferTexture2D(GL_FRAMEBUFFER, (GLenum)(GL_COLOR_ATTACHMENT0 + i), GL_TEXTURE_2D, 0, 0);
				}
				
				_boundRenderTargets = 0;
			}
			
			if(_stencilbuffer && !(_format & BufferFormatStencil))
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
				_stencilbuffer = 0;
			}
			
			if(_depthbuffer && !(_format & BufferFormatDepth || _format & BufferFormatStencil))
			{
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
				glDeleteRenderbuffers(1, &_depthbuffer);
				_depthbuffer = 0;
			}
			
			// Create new buffers
			if(_depthbuffer == 0 && (_format & BufferFormatDepth))
			{
				glGenRenderbuffers(1, &_depthbuffer);
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthbuffer);
				
				if(_format & BufferFormatStencil)
				{
					_stencilbuffer = _depthbuffer;
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _stencilbuffer);
				}
			}
			
			_formatChanged = false;
			_frameChanged  = true;
		}
		
		if(_renderTargetsChanged && (_format & BufferFormatColor))
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
			_renderTargetsChanged = false;
			
			UpdateDrawBuffers(_boundRenderTargets);
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
			if(_format & BufferFormatDepth)
			{
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
			}
			
			if(_format & BufferFormatStencil)
			{
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
			}
#endif
			
#if RN_TARGET_OPENGL_ES
			if(_format & BufferFormatDepth)
			{
				glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, width, height);
				
				RN_CHECKOPENGL();
			}
			
			if(_format & BufferFormatStencil)
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
		Camera(size, target, FlagDefaults)
	{}
	
	Camera::Camera(const Vector2& size, Texture *target, Flags flags) :
		Camera(size, target, flags, RenderStorage::BufferFormatComplete)
	{}
	
	
	Camera::Camera(const Vector2& size, Texture::Format targetFormat) :
		Camera(size, targetFormat, FlagDefaults)
	{}
	
	Camera::Camera(const Vector2& size, Texture::Format targetFormat, Flags flags) :
		Camera(size, targetFormat, flags, RenderStorage::BufferFormatComplete)
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
		
		SetRenderStorage(storage);
		Initialize();
		
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
		
		_lightTiles = Vector2(32, 32);
		
		Update(0.0f);
		UpdateProjection();
		UpdateFrustum();
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
			glClearStencil(0);
			glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
#endif
#if RN_PLATFORM_MAC_OS
			glClearDepth(1.0f);
			glClearStencil(0);
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
			
			if(_flags & FlagUpdateStorageFrame)
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
		
		if(_flags & FlagUpdateStorageFrame)
			_storage->SetFrame(_frame);
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
	
	Vector3 Camera::CamToWorld(Vector3 dir)
	{
		Vector4 vec(dir.x, dir.y, dir.z, 1.0f);
		vec = inverseProjectionMatrix->Transform(vec);
		vec /= vec.w;
		
		Vector3 temp;
		temp.x = vec.x;
		temp.y = vec.y;
		temp.z = vec.z;
		temp = inverseViewMatrix->Transform(temp);
		return temp;
	}
	
	void Camera::UpdateFrustum()
	{
		Vector3 pos2 = CamToWorld(Vector3(-1.0f, 1.0f, 1.0f));
		Vector3 pos3 = CamToWorld(Vector3(-1.0f, -1.0f, 1.0f));
		Vector3 pos5 = CamToWorld(Vector3(1.0f, 1.0f, 1.0f));
		Vector3 pos6 = CamToWorld(Vector3(1.0f, -1.0f, 1.0f));
		
		Vector3 vmax;
		Vector3 vmin;
		vmax.x = fmax(_position.x, fmax(pos2.x, fmax(pos3.x, fmax(pos5.x, pos6.x))));
		vmax.y = fmax(_position.y, fmax(pos2.y, fmax(pos3.y, fmax(pos5.y, pos6.y))));
		vmax.z = fmax(_position.z, fmax(pos2.z, fmax(pos3.z, fmax(pos5.z, pos6.z))));
		vmin.x = fmin(_position.x, fmin(pos2.x, fmin(pos3.x, fmin(pos5.x, pos6.x))));
		vmin.y = fmin(_position.y, fmin(pos2.y, fmin(pos3.y, fmin(pos5.y, pos6.y))));
		vmin.z = fmin(_position.z, fmin(pos2.z, fmin(pos3.z, fmin(pos5.z, pos6.z))));
		
		_frustumCenter = vmax+vmin;
		_frustumCenter = _frustumCenter*0.5f;
		_frustumRadius = _frustumCenter.Distance(vmax);
		
		_frustumLeft.SetPlane(_position, pos2, pos3);
		_frustumRight.SetPlane(_position, pos5, pos6);
		_frustumTop.SetPlane(_position, pos2, pos5);
		_frustumBottom.SetPlane(_position, pos3, pos6);
	}
	
	bool Camera::InFrustum(Vector3 &position, float &radius)
	{
		if(_frustumCenter.Distance(position) > _frustumRadius+radius)
			return false;
		
		if(_frustumLeft.Distance(position) > radius)
			return false;
		
		if(_frustumRight.Distance(position) < -radius)
			return false;
		
		if(_frustumTop.Distance(position) < -radius)
			return false;
		
		if(_frustumBottom.Distance(position) > radius)
			return false;
		
		return true;
	}
}
