//
//  RNCamera.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCamera.h"
#include "RNThread.h"
#include "RNKernel.h"
#include "RNWorld.h"
#include "RNLight.h"

namespace RN
{
	RNDeclareMeta(Camera)
	
	Camera::Camera(const Vector2& size) :
	Camera(size, TextureParameter::Format::RGBA8888)
	{}


	Camera::Camera(const Vector2& size, Texture *target) :
		Camera(size, target, FlagDefaults)
	{}

	Camera::Camera(const Vector2& size, Texture *target, Flags flags) :
		Camera(size, target, flags, RenderStorage::BufferFormatComplete)
	{}


	Camera::Camera(const Vector2& size, TextureParameter::Format targetFormat) :
		Camera(size, targetFormat, FlagDefaults)
	{}

	Camera::Camera(const Vector2& size, TextureParameter::Format targetFormat, Flags flags) :
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

	Camera::Camera(const Vector2& size, TextureParameter::Format targetFormat, Flags flags, RenderStorage::BufferFormat format) :
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
		World::SharedInstance()->RemoveCamera(this);
		
		if(_storage)
			_storage->Release();
		
		if(_skycube)
			_skycube->Release();
		
		if(_material)
			_material->Release();
		
		if(_stage)
			_stage->Release();
		
		if(_depthTiles)
			_depthTiles->Release();
	}


	void Camera::Initialize()
	{
		aspect   = 1.0f;
		fov      = 70.0f;

		clipnear = 0.1f;
		clipfar  = 500.0f;
		
		ortholeft = -100.0f;
		orthoright = 100.0f;
		orthobottom = -100.0f;
		orthotop = 100.0f;

		_clearColor  = Color(0.193f, 0.435f, 0.753f, 1.0f);
		_scaleFactor = Kernel::SharedInstance()->ScaleFactor();

		_material = 0;
		_stage = 0;

		_lightTiles = Vector2(32, 32);
		_depthTiles = 0;
		_skycube = 0;
		
		_depthFrame = 0;
		_depthArray = 0;
		_depthSize  = 0;
		
		_maxLights = 500;
		
		_allowDepthWrite = true;
		_lodCamera = 0;
		_useInstancing = true;
		
		_clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
		_colorMask = ColorFlagRed | ColorFlagGreen | ColorFlagBlue | ColorFlagAlpha;
		
		renderGroup = RenderGroup0;

		if(_flags & FlagUpdateStorageFrame)
			_storage->SetFrame(_frame);
		
		Update(0.0f);
		UpdateProjection();
		UpdateFrustum();
		
		World::SharedInstance()->AddCamera(this);
	}



	void Camera::Bind()
	{
		_storage->Bind();
	}

	void Camera::Unbind()
	{
		_storage->Unbind();
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
		if(_material)
			_material->Release();
		
		_material = material ? material->Retain() : 0;
	}

	void Camera::SetRenderStorage(RenderStorage *storage)
	{
		RN_ASSERT(storage, "Render storage mustn't be NULL!");

		if(_storage)
			_storage->Release();
		
		_storage =storage->Retain();

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
		if(_skycube)
			_skycube->Release();
		
		_skycube = skycube ? skycube->Retain() : 0;
	}
	
	void Camera::SetMaxLightsPerTile(machine_uint lights)
	{
		_maxLights = lights;
	}
	
	void Camera::SetCameraFlags(Flags flags)
	{
		_flags = flags;
	}
	
	void Camera::SetLODCamera(Camera *camera)
	{
		_lodCamera = camera;
	}
	
	void Camera::SetUseInstancing(bool activate)
	{
		_useInstancing = activate;
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
		if(_stage)
			_stage->Release();
		
		_stage = stage ? stage->Retain() : 0;

		UpdateProjection();
		World::SharedInstance()->RemoveSceneNode(stage);
		World::SharedInstance()->RemoveCamera(stage);
	}

	void Camera::ReplaceStage(Camera *stage)
	{
		Camera *temp = _stage ? _stage->_stage->Retain() : 0;
		World::SharedInstance()->AddSceneNode(_stage);
		World::SharedInstance()->AddCamera(_stage);

		if(_stage)
			_stage->Release();
		
		_stage = stage ? stage->Retain() : 0;

		if(_stage->_stage)
			_stage->RemoveStage(_stage->_stage);

		_stage->_stage = temp;

		UpdateProjection();
		World::SharedInstance()->RemoveSceneNode(stage);
		World::SharedInstance()->RemoveCamera(stage);
	}

	void Camera::RemoveStage(Camera *stage)
	{
		if(_stage == stage)
		{
			stage = stage->_stage ? stage->_stage->Retain() : 0;
			
			World::SharedInstance()->RemoveSceneNode(stage);
			World::SharedInstance()->RemoveCamera(stage);

			_stage->Release();
			_stage = stage;

			return;
		}

		Camera *temp = _stage;
		while(temp)
		{
			if(temp->_stage == stage)
			{
				stage = stage->_stage ? stage->_stage->Retain() : 0;
				World::SharedInstance()->AddSceneNode(stage);
				World::SharedInstance()->AddCamera(stage);

				temp->_stage->Release();
				temp->_stage = stage;

				return;
			}

			temp = temp->_stage;
		}
	}
	
	Matrix Camera::MakeShadowSplit(Camera *camera, Light *light, float near, float far)
	{
		Vector3 nearcenter = camera->ToWorldZ(Vector3(0.0f, 0.0f, near));
		Vector3 farcorner1 = camera->ToWorldZ(Vector3(1.0f, 1.0f, far));
		Vector3 farcorner2 = camera->ToWorldZ(Vector3(-1.0f, -1.0f, far));
		Vector3 farcenter = (farcorner1+farcorner2)*0.5f;
		
		Vector3 center = (nearcenter+farcenter)*0.5f;
		float dist = center.Distance(farcorner1);
		
		Vector3 pixelsize = Vector3(Vector2(dist*2.0f), 1.0f)/Vector3(_frame.width, _frame.height, 1.0f);
		Vector3 pos = center+light->Forward()*500.0f;
		
		Matrix rot = light->WorldRotation().RotationMatrix();
		pos = rot.Inverse()*pos;
		
		pos /= pixelsize;
		pos.x = floorf(pos.x);
		pos.y = floorf(pos.y);
		pos.z = floorf(pos.z);
		pos *= pixelsize;
		pos = rot*pos;
		SetPosition(pos);
		
		clipfar = 500.0f+dist*2.0f;
		ortholeft = -dist;
		orthoright = dist;
		orthobottom = -dist;
		orthotop = dist;
		UpdateProjection();
		
	/*	Vector3 nearcorner1 = camera->ToWorldZ(Vector3(-1.0f, -1.0f, near));
		Vector3 farcorner1 = camera->ToWorldZ(Vector3(1.0f, 1.0f, far));
		Vector3 farcorner2 = camera->ToWorldZ(Vector3(-1.0f, -1.0f, far));
		float dist1 = nearcorner1.Distance(farcorner1);
		float dist2 = farcorner1.Distance(farcorner2);
		float dist = 0.0f;
		Vector3 center;
		if(dist1 < dist2)
		{
			dist = dist2;
			center = (farcorner2+farcorner1)*0.5f;
		}
		else
		{
			dist = dist1;
			center = (nearcorner1+farcorner1)*0.5f;
		}
		
		ortholeft = -dist;
		orthoright = dist;
		orthobottom = -dist;
		orthotop = dist;
		UpdateProjection();
		
		Vector3 pixelsize = Vector3(Vector2(dist*2.0f), 1.0f)/Vector3(_frame.width, _frame.height, 1.0f);
		Vector3 pos = center+light->Forward()*200.0f;
		
		Matrix rot = light->WorldRotation().RotationMatrix();
		pos = rot.Inverse()*pos;
		
		pos /= pixelsize;
		pos.x = floorf(pos.x);
		pos.y = floorf(pos.y);
		pos.z = floorf(pos.z);
		pos *= pixelsize;
		pos = rot*pos;
		SetPosition(pos);*/
		
		Matrix projview = projectionMatrix*WorldTransform().Inverse();
		return projview;
	}
	
	void Camera::ActivateTiledLightLists(Texture *depthTiles)
	{
		if(_depthTiles)
			_depthTiles->Release();
		
		_depthTiles = depthTiles ? depthTiles->Retain() : 0;
	}
	
	// Helper
	void Camera::Update(float delta)
	{
		if(_stage)
			_stage->Update(delta);
		
		SceneNode::Update(delta);
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
				_stage->SetWorldPosition(WorldPosition());
				_stage->SetWorldRotation(WorldRotation());
			}

			_stage->PostUpdate();
		}
	}

	void Camera::UpdateProjection()
	{
		if(_flags & FlagOrthogonal)
		{
			projectionMatrix.MakeProjectionOrthogonal(ortholeft, orthoright, orthobottom, orthotop, clipnear, clipfar);
			return;
		}
		
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
		if(_flags & FlagOrthogonal)
		{
			Vector4 temp = vec*0.5f;
			temp += 0.5f;
			Vector4 temp2(1.0f-temp.x, 1.0f-temp.y, 1.0f-temp.z, 0.0f);
			vec = Vector4(ortholeft, orthobottom, clipnear, 1.0f)*temp2;
			vec += Vector4(orthoright, orthotop, clipfar, 1.0f)*temp;
			vec.z *= -1.0f;
		}
		
		vec = inverseProjectionMatrix.Transform(vec);
		vec /= vec.w;

		Vector3 temp(vec.x, vec.y, vec.z);
		temp = inverseViewMatrix.Transform(temp);
		
		return temp;
	}
	
	Vector3 Camera::ToWorldZ(const Vector3& dir)
	{
		Vector4 vec(dir.x, dir.y, 1.0, 1.0f);
		if(_flags & FlagOrthogonal)
		{
			Vector2 temp(dir.x*0.5+0.5, dir.y*0.5+0.5);
			vec.x = ortholeft*(1.0f-temp.x)+orthoright*temp.x;
			vec.y = orthobottom*(1.0f-temp.y)+orthotop*temp.y;
			vec.z *= -1.0f;
		}
		
		vec = inverseProjectionMatrix.Transform(vec);
		vec = vec.Normalize()*dir.z;
		
		Vector3 temp(vec.x, vec.y, vec.z);
		temp = inverseViewMatrix.Transform(temp);
		
		return temp;
	}
	
	const Rect& Camera::Frame()
	{
		if(_flags & FlagFullscreen)
		{
			Rect frame = Kernel::SharedInstance()->Window()->Frame();
			SetFrame(frame);
		}
		
		return _frame;
	}
	
	float *Camera::DepthArray()
	{
		if(!_depthTiles)
			return 0;
		
		if(Kernel::SharedInstance()->CurrentFrame() == LastFrame())
			return _depthArray;
		
		int width  = (int)_depthTiles->Width();
		int height = (int)_depthTiles->Height();
		
		size_t size = width * height * 2;
		if(size > _depthSize)
		{
			delete _depthArray;
			
			_depthArray = new float[size];
			_depthSize  = size;
		}
		
		_depthTiles->Bind();
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, _depthArray);
		_depthTiles->Unbind();
		
		return _depthArray;
	}

	void Camera::UpdateFrustum()
	{
		Vector3 pos2 = ToWorld(Vector3(-1.0f, 1.0f, 1.0f));
		Vector3 pos3 = ToWorld(Vector3(-1.0f, -1.0f, 1.0f));
		Vector3 pos5 = ToWorld(Vector3(1.0f, 1.0f, 1.0f));
		Vector3 pos6 = ToWorld(Vector3(1.0f, -1.0f, 1.0f));
		
		const Vector3& position = WorldPosition();
		Vector3 direction = WorldRotation().RotateVector(RN::Vector3(0.0, 0.0, -1.0));

		Vector3 vmax;
		Vector3 vmin;
		vmax.x = MAX(position.x, MAX(pos2.x, MAX(pos3.x, MAX(pos5.x, pos6.x))));
		vmax.y = MAX(position.y, MAX(pos2.y, MAX(pos3.y, MAX(pos5.y, pos6.y))));
		vmax.z = MAX(position.z, MAX(pos2.z, MAX(pos3.z, MAX(pos5.z, pos6.z))));
		vmin.x = MIN(position.x, MIN(pos2.x, MIN(pos3.x, MIN(pos5.x, pos6.x))));
		vmin.y = MIN(position.y, MIN(pos2.y, MIN(pos3.y, MIN(pos5.y, pos6.y))));
		vmin.z = MIN(position.z, MIN(pos2.z, MIN(pos3.z, MIN(pos5.z, pos6.z))));

		_frustumCenter = vmax + vmin;
		_frustumCenter = _frustumCenter * 0.5f;
		_frustumRadius = _frustumCenter.Distance(vmax);

		_frustumLeft.SetPlane(position, pos2, pos3, 1.0f);
		_frustumRight.SetPlane(position, pos5, pos6, -1.0f);
		_frustumTop.SetPlane(position, pos2, pos5, -1.0f);
		_frustumBottom.SetPlane(position, pos3, pos6, 1.0f);
		_frustumNear.SetPlane(position + direction * clipnear, -direction);
		_frustumFar.SetPlane(position + direction * clipfar, direction);
		
#define CopyAndAbsPlane(dest, source) \
		dest = source; \
		dest.normal.x = Math::FastAbs(dest.normal.x); \
		dest.normal.y = Math::FastAbs(dest.normal.y); \
		dest.normal.z = Math::FastAbs(dest.normal.z)
		
		CopyAndAbsPlane(_absFrustumLeft, _frustumLeft);
		CopyAndAbsPlane(_absFrustumRight, _frustumRight);
		CopyAndAbsPlane(_absFrustumTop, _frustumTop);
		CopyAndAbsPlane(_absFrustumBottom, _frustumBottom);
		CopyAndAbsPlane(_absFrustumFar, _frustumFar);
		CopyAndAbsPlane(_absFrustumNear, _frustumNear);
		
#undef CopyAndAbsPlane
		
	}

	bool Camera::InFrustum(const Vector3& position, float radius)
	{
		if(_frustumCenter.Distance(position) > _frustumRadius + radius)
			return false;

		if(_frustumLeft.Distance(position) > radius)
			return false;

		if(_frustumRight.Distance(position) > radius)
			return false;

		if(_frustumTop.Distance(position) > radius)
			return false;

		if(_frustumBottom.Distance(position) > radius)
			return false;
		
		if(_frustumNear.Distance(position) > radius)
			return false;
		
		if(_frustumFar.Distance(position) > radius)
			return false;

		return true;
	}
	
	bool Camera::InFrustum(const Sphere& sphere)
	{
		return InFrustum(sphere.Position(), sphere.radius);
	}
	
	bool Camera::InFrustum(const AABB& aabb)
	{
		Plane *planes = &_frustumLeft;
		Plane *absPlanes = &_absFrustumLeft;
	
		Vector3 position = aabb.Position();
		
		for(int i=0; i<6; i++)
		{
			const Plane& plane = planes[i];
			const Plane& absPlane = absPlanes[i];
			
			float d = position.Dot(plane.normal);
			float r = aabb.width.Dot(absPlane.normal);
			
			float dpr = d + r + plane.d;
			
			if(Math::IsNegative(dpr))
				return false;
		}
		
		return true;
	}
	
	bool Camera::IsVisibleInCamera(Camera *camera)
	{
		return true;
	}
}
