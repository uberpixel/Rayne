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
#include "RNWorld.h"

namespace RN
{
	RNDeclareMeta(Camera)
	
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
		_flags(flags)
	{
		_storage = 0;
		
		RenderStorage *storage = new RenderStorage(format);
		storage->AddRenderTarget(target);
		
		SetRenderStorage(storage);
		Initialize();
	}

	Camera::Camera(const Vector2& size, Texture::Format targetFormat, Flags flags, RenderStorage::BufferFormat format) :
		_frame(Vector2(0.0f, 0.0f), size),
		_flags(flags)
	{
		_storage = 0;

		RenderStorage *storage = new RenderStorage(format);
		storage->AddRenderTarget(targetFormat);
		
		SetRenderStorage(storage);
		Initialize();
	}

	Camera::Camera(const Vector2& size, RenderStorage *storage, Flags flags) :
		_frame(Vector2(0.0f, 0.0f), size),
		_flags(flags)
	{
		_storage = 0;

		SetRenderStorage(storage);
		Initialize();
	}

	Camera::~Camera()
	{
		_storage->Release();
		_skycube->Release();
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

		if(_flags & FlagUpdateStorageFrame)
			_storage->SetFrame(_frame);
		
		Update(0.0f);
		UpdateProjection();
		UpdateFrustum();
	}



	void Camera::Bind()
	{
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
		_material = material->Retain();
	}

	void Camera::SetRenderStorage(RenderStorage *storage)
	{
		RN_ASSERT(storage, "Render storage mustn't be NULL!");

		_storage->Release();
		_storage = storage->Retain();

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
	
	void Camera::SetSkyCube(Model *skycube)
	{
		_skycube->Release();
		_skycube = skycube->Retain();
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
		_stage = stage->Retain();

		UpdateProjection();
		World::SharedInstance()->RemoveTransform(stage);
	}

	void Camera::ReplaceStage(Camera *stage)
	{
		Camera *temp = _stage ? _stage->_stage->Retain() : 0;
		World::SharedInstance()->AddTransform(_stage);

		_stage->Release();
		_stage = stage->Retain();

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
			stage = stage->_stage->Retain();
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
				stage = stage->_stage->Retain();
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
		UpdateFrustum();
		
		inverseViewMatrix = WorldTransform();
		viewMatrix = inverseViewMatrix.Inverse();
		
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

		projectionMatrix.MakeProjectionPerspective(fov, aspect, clipnear, clipfar);
		inverseProjectionMatrix.MakeInverseProjectionPerspective(fov, aspect, clipnear, clipfar);

		if(_stage && _stage->_flags & FlagInheritProjection)
		{
			_stage->aspect = aspect;
			_stage->fov    = fov;

			_stage->clipfar  = clipfar;
			_stage->clipnear = clipnear;

			_stage->UpdateProjection();
		}
	}

	Vector3 Camera::ToWorld(const Vector3& dir)
	{
		Vector4 vec(dir.x, dir.y, dir.z, 1.0f);
		vec = inverseProjectionMatrix.Transform(vec);
		vec /= vec.w;

		Vector3 temp(vec.x, vec.y, vec.z);
		temp = inverseViewMatrix.Transform(temp);
		
		return temp;
	}
	
	const Rect& Camera::Frame()
	{
		if(_flags & FlagFullscreen)
		{
			Rect frame = Kernel::SharedInstance()->Window()->Frame();
			return frame;
		}
		
		return _frame;
	}

	void Camera::UpdateFrustum()
	{
		Vector3 pos2 = ToWorld(Vector3(-1.0f, 1.0f, 1.0f));
		Vector3 pos3 = ToWorld(Vector3(-1.0f, -1.0f, 1.0f));
		Vector3 pos5 = ToWorld(Vector3(1.0f, 1.0f, 1.0f));
		Vector3 pos6 = ToWorld(Vector3(1.0f, -1.0f, 1.0f));

		Vector3 vmax;
		Vector3 vmin;
		vmax.x = MAX(_position.x, MAX(pos2.x, MAX(pos3.x, MAX(pos5.x, pos6.x))));
		vmax.y = MAX(_position.y, MAX(pos2.y, MAX(pos3.y, MAX(pos5.y, pos6.y))));
		vmax.z = MAX(_position.z, MAX(pos2.z, MAX(pos3.z, MAX(pos5.z, pos6.z))));
		vmin.x = MIN(_position.x, MIN(pos2.x, MIN(pos3.x, MIN(pos5.x, pos6.x))));
		vmin.y = MIN(_position.y, MIN(pos2.y, MIN(pos3.y, MIN(pos5.y, pos6.y))));
		vmin.z = MIN(_position.z, MIN(pos2.z, MIN(pos3.z, MIN(pos5.z, pos6.z))));

		_frustumCenter = vmax + vmin;
		_frustumCenter = _frustumCenter * 0.5f;
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
