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
		
		override = OverrideAll;

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
		
		_frameID = 0;
		_depthFrame = 0;
		_depthArray = 0;
		_depthSize  = 0;
		
		_maxLights = 500;
		
		_allowDepthWrite = true;
		
		_clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
		_colorMask = ColorFlagRed | ColorFlagGreen | ColorFlagBlue | ColorFlagAlpha;

		if(_flags & FlagUpdateStorageFrame)
			_storage->SetFrame(_frame);
		
		Update(0.0f);
		UpdateProjection();
		UpdateFrustum();
		
		World::SharedInstance()->AddCamera(this);
	}



	void Camera::Bind()
	{
		Thread *thread = Thread::CurrentThread();

		if(thread->CurrentCamera() != this)
			glBindFramebuffer(GL_FRAMEBUFFER, _storage->_framebuffer);
		
		_frameID ++;
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
		World::SharedInstance()->RemoveTransform(stage);
		World::SharedInstance()->RemoveCamera(stage);
	}

	void Camera::ReplaceStage(Camera *stage)
	{
		Camera *temp = _stage ? _stage->_stage->Retain() : 0;
		World::SharedInstance()->AddTransform(_stage);
		World::SharedInstance()->AddCamera(_stage);

		if(_stage)
			_stage->Release();
		
		_stage = stage ? stage->Retain() : 0;

		if(_stage->_stage)
			_stage->RemoveStage(_stage->_stage);

		_stage->_stage = temp;

		UpdateProjection();
		World::SharedInstance()->RemoveTransform(stage);
		World::SharedInstance()->RemoveCamera(stage);
	}

	void Camera::RemoveStage(Camera *stage)
	{
		if(_stage == stage)
		{
			stage = stage->_stage ? stage->_stage->Retain() : 0;
			
			World::SharedInstance()->RemoveTransform(stage);
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
				World::SharedInstance()->AddTransform(stage);
				World::SharedInstance()->AddCamera(stage);

				temp->_stage->Release();
				temp->_stage = stage;

				return;
			}

			temp = temp->_stage;
		}
	}
	
	void Camera::MakeShadowSplit(Camera *camera, Light *light, float near, float far)
	{
		Vector3 pos2 = camera->ToWorld(Vector3(-1.0f, 1.0f, 1.0f));
		Vector3 pos3 = camera->ToWorld(Vector3(-1.0f, -1.0f, 1.0f));
		Vector3 pos5 = camera->ToWorld(Vector3(1.0f, 1.0f, 1.0f));
		Vector3 pos6 = camera->ToWorld(Vector3(1.0f, -1.0f, 1.0f));
		
		Vector3 vmax;
		Vector3 vmin;
		vmax.x = MAX(_position.x, MAX(pos2.x, MAX(pos3.x, MAX(pos5.x, pos6.x))));
		vmax.y = MAX(_position.y, MAX(pos2.y, MAX(pos3.y, MAX(pos5.y, pos6.y))));
		vmax.z = MAX(_position.z, MAX(pos2.z, MAX(pos3.z, MAX(pos5.z, pos6.z))));
		vmin.x = MIN(_position.x, MIN(pos2.x, MIN(pos3.x, MIN(pos5.x, pos6.x))));
		vmin.y = MIN(_position.y, MIN(pos2.y, MIN(pos3.y, MIN(pos5.y, pos6.y))));
		vmin.z = MIN(_position.z, MIN(pos2.z, MIN(pos3.z, MIN(pos5.z, pos6.z))));
	}
	
	//---------------------------------------------------------------------------
	//  CalcOrthoViewport{}
	//          lightMatrix - a full-featured light view matrix
	//          FovYDiv - a constant value that is used to increase the size of the light view frustum
	//
	//---------------------------------------------------------------------------
	void Camera::CalcOrthoViewport(Camera *camera, float nearSlice, float farSlice, Light *light, const float FovYDiv)
	{
        //
        // Grab the current camera matrix ... which is the camera the viewer uses
        // The idea is then to find the nearSlice and farSlice values to later snap around
        // the othographic viewport necessary for the light camera
        //
        // Get the position and the "basis" vectors from the viewer camera matrix
        //      camPos - d
        //      camDir - c
        //      camA - a - "x axis"
        //      camB - b - "y axis"
        Vector3 camPos(camera->Position());
        Vector3 camDir(camera->Forward());
        Vector3 camA(camera->Right());
        Vector3 camB(camera->Up());
		
        // normalize vectors
        camDir.Normalize();
        camA.Normalize();
        camB.Normalize();
		
        // get the fov x and y value
        float fovY = (camera->fov/2.0f) * FovYDiv;
        // a ratio of 1.0f just looks better ... maybe it fits better to the square nature of the render target
        float ratio = camera->aspect;
        float fovX = fovY * ratio;
		
        // convert from degrees to radians
        fovX *= M_PI/180.0f;
        fovY *= M_PI/180.0f;
		
        //
        // this is the width and height of the rectangle that fits into the near
		// and far slice from the point of the viewer
        //
        Vector2 camViewWidth;
        Vector2 camViewHeight;
        camViewWidth = Vector2(tanf(fovX) * nearSlice, tanf(fovX) * farSlice);
        camViewHeight = Vector2(tanf(fovY) * nearSlice, tanf(fovY) * farSlice);
		
        //
        // get two vectors on the near and two vectors on the far plane
        // each pair points up and left
        //
        // -------------
        // |     ^     |
        // |     |     |
        // |  <--      |
        // |           |
        // |           |
        // -------------
        //
        // left near point from viewer camera frustum
        Vector3 leftNear(camA);
        leftNear *= camViewWidth.x;
        // up near point from viewer camera frustum
        Vector3 upNear(-camB);
        upNear *= camViewHeight.x;
        // left far point from viewer camera frustum
        Vector3 leftFar(camA);
        leftFar *= camViewWidth.y;
        // up far point from viewer camera frustum
        Vector3 upFar(-camB);
        upFar *= camViewHeight.y;
		
        //
        // get two vectors that point in the view direction
		// to the near and far plane
        //
        Vector3 nearCenter(camPos);
        // add a vector that points in the camera direction and
        // is scaled to get the near center of the frustum
        nearCenter += camDir*nearSlice;
//        nearCenter += camDir*(-m_ViewportZShift));
		
        Vector3 farCenter(camPos);
        // add a vector that points in the camera direction and
        // is scaled to get the far center of the frustum
        farCenter += camDir*farSlice;
//        farCenter += camDir*(-m_ViewportZShift);
		
        //
        // Write the far center and near center value in a box with eight edges
		// This box does not have any width and height so far, only length
        //
        int i;
        Vector3 extents[8];
        for(i = 0; i < 4; i++)
        {
			// fill up with the near and far center points
			extents[i] = nearCenter;
			extents[i + 4] = farCenter;
        }
		
		//
        // now add width and height to this box
		//
		// store the four points for the near plane
        // creates based on the near center point the four points that form
        // the near plane of a viewing frustum
        //
        // x           x
        //
        //
        //
        //
        //
        // x           x
        //
        // go from near center point to the left and then up
        extents[0] += leftNear;  extents[0] += upNear;
        // go from near center point to the right and then up
        extents[1] += -leftNear;  extents[1] += upNear;
        // go from the near center point to the left and then down
        extents[2] += leftNear;  extents[2] += -upNear;
        // go from the near center point to the right and then down
        extents[3] += -leftNear;  extents[3] += -upNear;
		
        // Create far plane
        // creates based on the far center point the four points that form
        // the far plane of a viewing frustum
        // go from the far center point to the left and then up
        extents[4] += leftFar;  extents[4] += upFar;
        // go from the far center point to the right and then up
        extents[5] += -leftFar;  extents[5] += upFar;
        // go from the far center point to the left and then down
        extents[6] += leftFar;  extents[6] += -upFar;
        // go from the far center point to the right and then down
        extents[7] += -leftFar;  extents[7] += -upFar;
		
		//
		// now we have a box that represents one 3D slice of the view frustum
		// as shown in the figure above let's move this into light space
		//
        // grab view matrix constructed from the light direction
        // == light space matrix
		// normalize the three direction vectors
        Matrix lightOrientation = light->Rotation().RotationMatrix();
		lightOrientation = lightOrientation.Inverse();
		
        // Transforms the view frustum slice into light space
        // UnTransform() - Transform a vector by the inverse of the current matrix
        for(i = 0; i < 8; i++)
			extents[i] = lightOrientation * extents[i];
		
		//
		// Because the slice of the frustum has still the
		// shape of the viewers frustum, we need to create now
		// a box that is used in an orthographic projection
		//
        // Create axis aligned bounding box in light space
		//
        Vector3 minInLight,maxInLight;
        minInLight = extents[0];
        maxInLight = extents[0];
		
        for(i = 0; i < 8; i++)
        {
			// stores the smallest and largest numbers -> extents
			// in light view space
			if(extents[i].x < minInLight.x)
				minInLight.x = extents[i].x;
			if(extents[i].y < minInLight.y)
				minInLight.y = extents[i].y;
			if(extents[i].z < minInLight.z)
				minInLight.z = extents[i].z;
			if(extents[i].x > maxInLight.x)
				maxInLight.x = extents[i].x;
			if(extents[i].y > maxInLight.y)
				maxInLight.y = extents[i].y;
			if(extents[i].z > maxInLight.z)
				maxInLight.z = extents[i].z;
        }
        extents[0] = Vector3(minInLight.x, maxInLight.y, minInLight.z);
        extents[1] = Vector3(maxInLight.x, maxInLight.y, minInLight.z);
        extents[2] = Vector3(minInLight.x, minInLight.y, minInLight.z);
        extents[3] = Vector3(maxInLight.x, minInLight.y, minInLight.z);
        extents[4] = Vector3(minInLight.x, maxInLight.y, maxInLight.z);
        extents[5] = Vector3(maxInLight.x, maxInLight.y, maxInLight.z);
        extents[6] = Vector3(minInLight.x, minInLight.y, maxInLight.z);
        extents[7] = Vector3(maxInLight.x, minInLight.y, maxInLight.z);
		
		//
		// now we prepare the values vp.Ortho() expects
		//
        // Width is the magnitude of the vector from the near top left corner to the near top right corner
        Vector3 vWidth = extents[1] - extents[0];
        // Height is the magnitude of the vector from the near top left corner to the near bottom left corner
        Vector3 vHeight = extents[2] - extents[0];
        // Depth is the magnitude of the vector from the near top left corner to the far top left corner
        Vector3 vDepth = extents[4] - extents[0];
        float width = vWidth.Length();
        float height = vHeight.Length();
        float depth = vDepth.Length();
        float halfWidth = width * 0.5f;
        float halfHeight = height * 0.5f;
		
		//
		// Scale the depth
		//
		// This stretches the length of the box
		// so that it catches also large buildings like skycrapers
//        depth *= m_LightDepthScale;
		
		
		//
		// now we prepare the values SetCameraMtx() expects
		//
        // get the center of the box
        Vector3 vCameraPos;
        vCameraPos = extents[0]*0.5f+extents[7]*0.5f;
		
        // Transform the center of the box back into into world space
		// this way the box follows the camera
		lightOrientation = light->Rotation().RotationMatrix();
        vCameraPos = lightOrientation*vCameraPos;
        
        float fHalfDepth = depth * 0.5f;
        vCameraPos += light->Forward()*(-fHalfDepth);
		
		SetPosition(vCameraPos);
		
        // dynamic generation of the orthographic projection matrix
		ortholeft = halfWidth;
		orthoright = -halfWidth;
		orthobottom = -halfHeight;
		orthotop = halfHeight;
		
		clipnear = 0.0f;
		clipfar = depth;
		
		UpdateProjection();
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
			SetFrame(frame);
		}
		
		return _frame;
	}
	
	float *Camera::DepthArray()
	{
		if(!_depthTiles)
			return 0;
		
		if(_depthFrame == _frameID)
			return _depthArray;
		
		int tilesWidth  = (int)_lightTiles.x;
		int tilesHeight = (int)_lightTiles.y;
		
		size_t size = tilesWidth * tilesHeight * 2 * sizeof(float);
		if(size > _depthSize)
		{
			delete _depthArray;
			
			_depthArray = new float[size];
			_depthSize  = size;
		}
		
		_depthTiles->Bind();
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, _depthArray);
		_depthTiles->Unbind();
		
		_depthFrame = _frameID;
		return _depthArray;
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
