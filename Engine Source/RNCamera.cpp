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
	RenderStorage::RenderStorage(BufferFormat format, Texture *depthTexture)
	{
		_formatChanged = true;
		_frameChanged  = true;
		_renderTargetsChanged = true;

		_boundRenderTargets = 0;
		_renderTargets = new Array<Texture>();
		_depthTexture = depthTexture->Retain<Texture>();

		_framebuffer = _depthbuffer = _stencilbuffer = 0;
		_scaleFactor = Kernel::SharedInstance()->ScaleFactor();
		_format      = format;

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
	

	void RenderStorage::SetRenderTarget(Texture *target, uint32 index)
	{
		RN_ASSERT0(_format & BufferFormatColor);
		
		target->Bind();
		target->SetLinear(true);
		target->SetGeneratesMipmaps(false);
		target->SetWrappingMode(Texture::WrapModeClamp);
		target->SetFilter(Texture::FilterNearest);
		target->Unbind();

		_renderTargets->ReplaceObjectAtIndex(index, target);
		_renderTargetsChanged = true;
	}

	void RenderStorage::RemoveRenderTarget(Texture *target)
	{
		RN_ASSERT0(_format & BufferFormatColor);
		
		_renderTargets->RemoveObject(target);
		_renderTargetsChanged = true;
	}

	void RenderStorage::AddRenderTarget(Texture *target)
	{
		RN_ASSERT0(_format & BufferFormatColor);
		
		if(_renderTargets->Count() >= MaxRenderTargets())
			throw ErrorException(0, 0, 0);

		target->Bind();
		target->SetLinear(true);
		target->SetGeneratesMipmaps(false);
		target->SetWrappingMode(Texture::WrapModeClamp);
		target->SetFilter(Texture::FilterNearest);
		target->Unbind();

		_renderTargets->AddObject(target);
		_renderTargetsChanged = true;
	}

	void RenderStorage::AddRenderTarget(Texture::Format format)
	{
		RN_ASSERT0(_format & BufferFormatColor);
		
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
	
	void RenderStorage::SetDepthTarget(Texture *depthTexture)
	{
		if(depthTexture)
		{
			RN_ASSERT(_format & BufferFormatDepth, "Depth textures are only supported for render storages with depth support");
			
			if(_format & BufferFormatStencil)
			{
				RN_ASSERT0(depthTexture->TextureFormat() == Texture::FormatDepthStencil);
			}
			else
			{
				RN_ASSERT0(depthTexture->TextureFormat() == Texture::FormatDepth);
			}
		}
		
		_depthTexture->Release();
		_depthTexture = depthTexture->Retain<Texture>();
		
		_formatChanged = true;
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
			if(_depthTexture)
			{
				if(_stencilbuffer)
				{
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
					_stencilbuffer = 0;
				}

				if(_depthbuffer)
				{
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
					glDeleteRenderbuffers(1, &_depthbuffer);
					_depthbuffer = 0;
				}
			}

			// Create new buffers
			if((_depthbuffer == 0 && (_format & BufferFormatDepth)) && !_depthTexture)
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

			if(_depthTexture)
			{
				if(_format & BufferFormatDepth)
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->Name(), 0);

				if(_format & BufferFormatStencil)
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->Name(), 0);
			}

			_formatChanged = false;
			_frameChanged  = true;
		}

		if(_renderTargetsChanged && (_format & BufferFormatColor))
		{
			// Unbind no longer used render targets
			for(machine_uint i=_renderTargets->Count(); i<_boundRenderTargets; i++)
			{
				GLenum attachment = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, 0, 0);
			}
			
			// Bind all render targetst to the framebuffer
			for(machine_uint i=0; i<_renderTargets->Count(); i++)
			{
				Texture *texture = _renderTargets->ObjectAtIndex(i);
				GLenum attachment = (GLenum)(GL_COLOR_ATTACHMENT0 + i);
				
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture->Name(), 0);
			}

			_boundRenderTargets = (uint32)_renderTargets->Count();
			_renderTargetsChanged = false;

			UpdateDrawBuffers(_boundRenderTargets);
		}

		// Allocate storage for the buffers
		if(_frameChanged)
		{
			uint32 width  = (uint32)(_frame.width  * _scaleFactor);
			uint32 height = (uint32)(_frame.height * _scaleFactor);

			for(machine_uint i=0; i<_renderTargets->Count(); i++)
			{
				Texture *texture = _renderTargets->ObjectAtIndex(i);

				texture->Bind();
				texture->SetData(0, width, height, Texture::FormatRGBA8888);
				texture->Unbind();
			}

			if(_depthTexture)
			{
				_depthTexture->Bind();
				
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
				
				_depthTexture->SetData(0, width, height, Texture::FormatRGBA8888);
				_depthTexture->Unbind();
			}
			else
			{
				if(!(_format & BufferFormatStencil))
				{
					glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
					glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
				}
				else
				{
					glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer);
					glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
				}
			}

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
		RenderingResource("Camera"),
		Transform(Transform::TransformTypeCamera)
	{
		_storage = 0;

		RenderStorage *storage = new RenderStorage(format);
		storage->AddRenderTarget(target);

		SetRenderStorage(storage);
		Initialize();
	}

	Camera::Camera(const Vector2& size, Texture::Format targetFormat, Flags flags, RenderStorage::BufferFormat format) :
		_frame(Vector2(0.0f, 0.0f), size),
		_flags(flags),
		RenderingResource("Camera"),
		Transform(Transform::TransformTypeCamera)
	{
		_storage = 0;

		RenderStorage *storage = new RenderStorage(format);
		storage->AddRenderTarget(targetFormat);

		SetRenderStorage(storage);
		Initialize();
	}

	Camera::Camera(const Vector2& size, RenderStorage *storage, Flags flags) :
		_frame(Vector2(0.0f, 0.0f), size),
		_flags(flags),
		RenderingResource("Camera"),
		Transform(Transform::TransformTypeCamera)
	{
		_storage = 0;

		SetRenderStorage(storage);
		Initialize();
	}

	Camera::~Camera()
	{
		_storage->Release();
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
		_depthTiles = 0;
		_skycube = 0;
		
		_allowDepthWrite = true;
		
		_clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
		_colorMask = ColorFlagRed | ColorFlagGreen | ColorFlagBlue | ColorFlagAlpha;

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
			Context *context = Context::ActiveContext();
			
			context->SetDepthClear(1.0f);
			context->SetStencilClear(0);
			context->SetClearColor(_clearColor);
			
			glClear(_clearMask);
		}
		
		glColorMask((_colorMask & ColorFlagRed), (_colorMask & ColorFlagGreen), (_colorMask & ColorFlagBlue), (_colorMask & ColorFlagAlpha));
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
	
	void Camera::SetClearMask(ClearFlags mask)
	{
		_clearMask = 0;
		
		if(mask & ClearFlagColor)
			_clearMask |= GL_COLOR_BUFFER_BIT;
		
		if(mask & ClearFlagDepth)
			_clearMask |= GL_DEPTH_BUFFER_BIT;
		
		if(mask & ClearFlagStencil)
			_clearMask |= GL_STENCIL_BUFFER_BIT;
	}
	
	void Camera::SetColorMask(ColorFlags mask)
	{
		_colorMask = mask;
	}
	
	void Camera::SetAllowsDepthWrite(bool flag)
	{
		_allowDepthWrite = flag;
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
		World::SharedInstance()->RemoveTransform(stage);
	}

	void Camera::ReplaceStage(Camera *stage)
	{
		Camera *temp = _stage ? _stage->_stage->Retain<Camera>() : 0;
		World::SharedInstance()->AddTransform(_stage);

		_stage->Release();
		_stage = stage->Retain<Camera>();

		if(_stage->_stage)
			_stage->RemoveStage(_stage->_stage);

		_stage->_stage = temp;

		UpdateProjection();
		World::SharedInstance()->RemoveTransform(stage);
	}

	void Camera::RemoveStage(Camera *stage)
	{
		if(_stage == stage)
		{
			stage = stage->_stage->Retain<Camera>();
			World::SharedInstance()->AddTransform(stage);

			_stage->Release();
			_stage = stage;

			return;
		}

		Camera *temp = _stage;
		while(temp)
		{
			if(temp->_stage == stage)
			{
				stage = stage->_stage->Retain<Camera>();
				World::SharedInstance()->AddTransform(stage);

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
		if(_stage)
			_stage->PostUpdate();
		
		Transform::Update(delta);
	}
	
	void Camera::PostUpdate()
	{
		Transform::PostUpdate();
		
		projectionMatrix.SynchronizePast();
		inverseProjectionMatrix.SynchronizePast();
		
		viewMatrix.SynchronizePast();
		inverseViewMatrix.SynchronizePast();
		
		inverseViewMatrix = WorldTransform();
		viewMatrix = inverseViewMatrix->Inverse();

		if(_flags & FlagFullscreen)
		{
			Rect frame = Kernel::SharedInstance()->Window()->Frame();
			SetFrame(frame);
		}

		if(_stage)
		{
			if(_stage->_flags & FlagInheritFrame)
			{
				_stage->SetFrame(_frame);
			}
			
			if(_stage->_flags & FlagInheritPosition)
			{
				_stage->SetPosition(Position());
				_stage->SetRotation(Rotation());
			}

			_stage->PostUpdate();
		}
	}

	void Camera::UpdateProjection()
	{
		if(_flags & FlagUpdateAspect)
			aspect = _frame.width / _frame.height;

		projectionMatrix->MakeProjectionPerspective(fov, aspect, clipnear, clipfar);
		inverseProjectionMatrix->MakeInverseProjectionPerspective(fov, aspect, clipnear, clipfar);

		if(_stage && _stage->_flags & FlagInheritProjection)
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
		vmax.x = fmax(_position->x, fmax(pos2.x, fmax(pos3.x, fmax(pos5.x, pos6.x))));
		vmax.y = fmax(_position->y, fmax(pos2.y, fmax(pos3.y, fmax(pos5.y, pos6.y))));
		vmax.z = fmax(_position->z, fmax(pos2.z, fmax(pos3.z, fmax(pos5.z, pos6.z))));
		vmin.x = fmin(_position->x, fmin(pos2.x, fmin(pos3.x, fmin(pos5.x, pos6.x))));
		vmin.y = fmin(_position->y, fmin(pos2.y, fmin(pos3.y, fmin(pos5.y, pos6.y))));
		vmin.z = fmin(_position->z, fmin(pos2.z, fmin(pos3.z, fmin(pos5.z, pos6.z))));

		_frustumCenter = vmax+vmin;
		_frustumCenter = _frustumCenter*0.5f;
		_frustumRadius = _frustumCenter.Distance(vmax);

		_frustumLeft.SetPlane(_position, pos2, pos3);
		_frustumRight.SetPlane(_position, pos5, pos6);
		_frustumTop.SetPlane(_position, pos2, pos5);
		_frustumBottom.SetPlane(_position, pos3, pos6);
	}

	bool Camera::InFrustum(const Vector3& position, float radius)
	{
		if(_frustumCenter.Distance(position) > _frustumRadius + radius)
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
