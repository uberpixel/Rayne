//
//  RNCamera.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCamera.h"
#include "../Rendering/RNRenderer.h"
#include "../Rendering/RNWindow.h"

namespace RN
{
	RNDefineMeta(Camera, SceneNode)

	Camera::Camera() :
		_framebuffer(nullptr),
		_cameraSceneEntry(this),
		_window(nullptr),
		_flags(0)
	{
		Initialize();
	}

	Camera::Camera(const Vector2 &size) :
		_framebuffer(new Framebuffer(size, Framebuffer::Options::PrivateStorage, Texture::Format::RGBA8888)),
		_cameraSceneEntry(this),
		_window(nullptr),
		_flags(0)
	{
		Initialize();
	}

/*	Camera::Camera(const Vector2 &size, Texture *target) :
		Camera(size, target, Flags::Defaults)
	{}

	Camera::Camera(const Vector2 &size, Texture *target, Flags flags) :
		Camera(size, target, flags, RenderStorage::BufferFormatComplete)
	{}

	Camera::Camera(const Vector2 &size, Texture::Format targetFormat) :
		Camera(size, targetFormat, Flags::Defaults)
	{}

	Camera::Camera(const Vector2 &size, Texture::Format targetFormat, Flags flags) :
		Camera(size, targetFormat, flags, RenderStorage::BufferFormatComplete)
	{}


	Camera::Camera(const Vector2 &size, Texture *target, Flags flags, RenderStorage::BufferFormat format) :
		_frame(Vector2(0.0f, 0.0f), size),
		_storage(nullptr),
		_lightManager(nullptr)
	{
		RenderStorage *storage = new RenderStorage(format);
		storage->AddRenderTarget(target);

		SetFlags(flags);
		SetRenderStorage(storage);
		Initialize();
	}

	Camera::Camera(const Vector2 &size, Texture::Format targetFormat, Flags flags, RenderStorage::BufferFormat format, float scaleFactor) :
		_frame(Vector2(0.0f, 0.0f), size),
		_scaleFactor(scaleFactor),
		_storage(nullptr),
		_lightManager(nullptr)
	{
		RenderStorage *storage = new RenderStorage(format, 0, scaleFactor);
		storage->AddRenderTarget(targetFormat);

		SetFlags(flags);
		SetRenderStorage(storage);
		Initialize();
	}

	Camera::Camera(const Vector2 &size, RenderStorage *storage, Flags flags, float scaleFactor) :
		_frame(Vector2(0.0f, 0.0f), size),
		_scaleFactor(scaleFactor),
		_storage(nullptr),
		_lightManager(nullptr)
	{
		SetFlags(flags);
		SetRenderStorage(storage);
		Initialize();
	}*/

	Camera::~Camera()
	{
		SafeRelease(_framebuffer);

/*		SafeRelease(_sky);
		SafeRelease(_material);

		for(PostProcessingPipeline *pipeline : _PPPipelines)
			pipeline->Release();

		if(_lightManager)
		{
			_lightManager->camera = nullptr;
			_lightManager->Release();
		}*/
	}


	void Camera::Initialize()
	{
		_fov      = 70.0f;
		_aspect   = 1.0f;

		_clipNear = 0.1f;
		_clipFar  = 500.0f;

		_orthoLeft   = -100.0f;
		_orthoRight  = 100.0f;
		_orthoBottom = -100.0f;
		_orthoTop    = 100.0f;

		_fogNear   = 100.0f;
		_fogFar    = 500.0f;
		_ambient   = Color(0.1f, 0.1f, 0.1f, 1.0f);
		_clipPlane = Plane();

		_dirtyProjection = true;
		_dirtyFrustum    = true;

		_clearColor  = Color(0.193f, 0.435f, 0.753f, 1.0f);
		_prefersLightManager = false;

//		_material   = nullptr;
//		_sky = nullptr;

		_priority  = 0;
		_lodCamera = nullptr;

		_prefersLightManager = true;
	}

	// Setter
	void Camera::SetFrame(const Rect &frame)
	{
		if(_frame != frame)
		{
			_frame = std::move(frame.GetIntegral());
			_dirtyProjection = true;
		}
	}

	void Camera::SetClearColor(const Color &clearColor)
	{
		_clearColor = clearColor;
	}

	void Camera::SetFlags(Flags flags)
	{
		_flags = flags;
	}

/*	void Camera::SetMaterial(Material *material)
	{
		SafeRelease(_material);
		_material = SafeRetain(material);
	}*/

/*	void Camera::SetSky(Model *sky)
	{
		SafeRelease(_sky);
		_sky = SafeRetain(sky);
	}*/

	void Camera::SetLODCamera(Camera *camera)
	{
		_lodCamera = camera;
	}

	void Camera::SetPriority(int32 priority)
	{
		_priority = priority;
	}

	void Camera::SetFOV(float fov)
	{
		_fov = fov;
		_dirtyProjection = true;
	}
	void Camera::SetAspectRatio(float ratio)
	{
		_aspect = ratio;
		_dirtyProjection = true;
	}

	void Camera::SetClipNear(float near)
	{
		_clipNear = near;
		_dirtyProjection = true;
		_dirtyFrustum = true;
	}
	void Camera::SetClipFar(float far)
	{
		_clipFar = far;
		_dirtyProjection = true;
		_dirtyFrustum = true;
	}

	void Camera::SetFogColor(Color color)
	{
		_fogColor = color;
	}
	void Camera::SetFogNear(float near)
	{
		_fogNear = near;
	}
	void Camera::SetFogFar(float far)
	{
		_fogFar = far;
	}

	void Camera::SetAmbientColor(Color color)
	{
		_ambient = color;
	}
	void Camera::SetClipPlane(const Plane &clipPlane)
	{
		_clipPlane = clipPlane;
	}

/*	void Camera::SetLightManager(LightManager *lightManager)
	{
		RN_ASSERT(!lightManager || !lightManager->camera, "The LightManager can't be attached to another camera!");

		if(_lightManager)
		{
			_lightManager->camera = nullptr;
			_lightManager->Release();
		}

		_prefersLightManager = false;

		_lightManager = SafeRetain(lightManager);

		if(_lightManager)
			_lightManager->camera = this;
	}*/

	void Camera::SetOrthogonalFrustum(float top, float bottom, float left, float right)
	{
		RN_ASSERT((_flags & Flags::Orthogonal), "SetOrthogonalFrustum() called, but the camera is not an orthogonal camera");

		_orthoLeft   = left;
		_orthoRight  = right;
		_orthoTop    = top;
		_orthoBottom = bottom;

		_dirtyProjection = true;
		_dirtyFrustum = true;
	}

	void Camera::SetProjectionMatrix(const Matrix &projectionMatrix)
	{
		_projectionMatrix = projectionMatrix;
		_dirtyFrustum = true;
		_dirtyProjection = false;
	}

	void Camera::DidUpdate(ChangeSet changeSet)
	{
		SceneNode::DidUpdate(changeSet);

		if(changeSet & ChangeSet::Position)
		{
			_dirtyFrustum = true;
		}
	}

	// Post Processing
/*	PostProcessingPipeline *Camera::AddPostProcessingPipeline(const std::string &name, int32 priority)
	{
		PostProcessingPipeline *pipeline = new PostProcessingPipeline(name, priority);
		try
		{
			AddPostProcessingPipeline(pipeline);
			return pipeline->Autorelease();
		}
		catch(Exception e)
		{
			delete pipeline;
			throw e;
		}
	}

	PostProcessingPipeline *Camera::GetPostProcessingPipeline(const std::string &name)
	{
		LockGuard<Object *> lock(this);

		auto iterator = _namedPPPipelines.find(name);
		return (iterator != _namedPPPipelines.end()) ? iterator->second : nullptr;
	}

	void Camera::AddPostProcessingPipeline(PostProcessingPipeline *pipeline)
	{
		LockGuard<Object *> lock(this);

		if(GetPostProcessingPipeline(pipeline->_name) || pipeline->host)
			throw Exception(Exception::Type::InvalidArgumentException, "A pipeline with this name already exists, or the pipeline is already associated with a camera!");

		_PPPipelines.push_back(pipeline);
		_namedPPPipelines.emplace(pipeline->_name, pipeline);

		pipeline->host = this;
		pipeline->Initialize();
		pipeline->Retain();

		std::stable_sort(_PPPipelines.begin(), _PPPipelines.end(), [&](PostProcessingPipeline *left, PostProcessingPipeline *right) {
			return (left->_priority < right->_priority);
		});
	}

	void Camera::RemovePostProcessingPipeline(PostProcessingPipeline *pipeline)
	{
		LockGuard<Object *> lock(this);

		for(auto i = _PPPipelines.begin(); i != _PPPipelines.end(); i ++)
		{
			if(*i == pipeline)
			{
				_PPPipelines.erase(i);
				_namedPPPipelines.erase(pipeline->_name);

				pipeline->host = nullptr;
				pipeline->Release();
				break;
			}
		}
	}*/

/*	Matrix Camera::MakeShadowSplit(Camera *camera, Light *light, float near, float far)
	{
		//Get camera frustum extends to be covered by the split
		Vector3 nearcenter = camera->ToWorld(Vector3(0.0f, 0.0f, near));
		Vector3 farcorner1 = camera->ToWorld(Vector3(1.0f, 1.0f, far));
		Vector3 farcorner2 = camera->ToWorld(Vector3(-1.0f, -1.0f, far));
		Vector3 farcenter = (farcorner1+farcorner2)*0.5f;
		Vector3 center = (nearcenter+farcenter)*0.5f;

		//Calculate the size of a pixel in world units
		float dist = center.GetDistance(farcorner1);
		Vector3 pixelsize = Vector3(Vector2(dist*2.0f), 1.0f)/Vector3(_frame.width, _frame.height, 1.0f);

		//Place the light camera 500 units above the splits center
		Vector3 pos = center-light->GetForward()*500.0f;

		//Transform the position to light space
		Matrix rot = light->GetWorldRotation().GetRotationMatrix();
		pos = rot.GetInverse()*pos;

		//Snap to the pixel grid
		pos /= pixelsize;
		pos.x = floorf(pos.x);
		pos.y = floorf(pos.y);
		pos.z = floorf(pos.z);
		pos *= pixelsize;

		//Transform back and place the camera there
		pos = rot*pos;
		SetPosition(pos);

		//Set the light camera frustum
		_clipFar = 500.0f + dist * 2.0f;
		_orthoLeft = -dist;
		_orthoRight = dist;
		_orthoBottom = -dist;
		_orthoTop = dist;

		//Update the projection matrix
		_dirtyProjection = true;
		UpdateProjection();

		//Return the resulting matrix
		Matrix projview = _projectionMatrix * GetWorldTransform().GetInverse();
		return projview;
	}*/

	// Helper
	void Camera::Update(float delta)
	{
		SceneNode::Update(delta);

		UpdateFrustum();

/*		for(auto i=_PPPipelines.begin(); i!=_PPPipelines.end(); i++)
		{
			PostProcessingPipeline *pipeline = *i;
			pipeline->PushUpdate(this, delta);
		}*/
	}

	void Camera::UpdateEditMode(float delta)
	{
		SceneNode::UpdateEditMode(delta);

		UpdateFrustum();

/*		for(auto i=_PPPipelines.begin(); i!=_PPPipelines.end(); i++)
		{
			PostProcessingPipeline *pipeline = *i;
			pipeline->PushUpdate(this, delta);
		}*/
	}

	void Camera::PostUpdate(Renderer *renderer)
	{
		_inverseViewMatrix = GetWorldTransform();
		_viewMatrix = _inverseViewMatrix.GetInverse();

		UpdateProjection(renderer);

/*		if(_flags & Flags::Fullscreen)
		{
			Vector2 size = Window::GetSharedInstance()->GetSize();
			SetFrame(Rect(Vector2(), size));
		}

		for(auto i=_PPPipelines.begin(); i!=_PPPipelines.end(); i++)
		{
			PostProcessingPipeline *pipeline = *i;
			pipeline->PostUpdate(this, GetWorldPosition(), GetWorldRotation(), _frame);
		}*/
	}

	void Camera::UpdateProjection(Renderer *renderer)
	{
		if(_flags & Flags::Orthogonal)
		{
			_projectionMatrix = Matrix::WithProjectionOrthogonal(_orthoLeft, _orthoRight, _orthoBottom, _orthoTop, _clipNear, _clipFar);
			return;
		}

		if(!_framebuffer)
		{
			if(!_window)
				_window = renderer->GetMainWindow();

			Vector2 size = _window->GetSize();

			_aspect = size.x / size.y;
		}
		else
		{
			_aspect = _frame.width / _frame.height;
		}

		_projectionMatrix = Matrix::WithProjectionPerspective(_fov, _aspect, _clipNear, _clipFar);
		_inverseProjectionMatrix = _projectionMatrix.GetInverse();

		_dirtyFrustum = true;
		UpdateFrustum();
	}

	void Camera::UpdateFrustum()
	{
		if(!_dirtyFrustum)
			return;

		_dirtyFrustum = false;

		Vector3 pos1 = __ToWorld(Vector3(-1.0f, 1.0f, 0.0f));
		Vector3 pos2 = __ToWorld(Vector3(-1.0f, 1.0f, 1.0));
		Vector3 pos3 = __ToWorld(Vector3(-1.0f, -1.0f, 1.0));
		Vector3 pos4 = __ToWorld(Vector3(1.0f, -1.0f, 0.0));
		Vector3 pos5 = __ToWorld(Vector3(1.0f, 1.0f, 1.0));
		Vector3 pos6 = __ToWorld(Vector3(1.0f, -1.0f, 1.0));

		const Vector3 &position = GetWorldPosition();
		Vector3 direction = GetWorldRotation().GetRotatedVector(Vector3(0.0, 0.0, -1.0));

		Vector3 vmax;
		Vector3 vmin;
		vmax.x = std::max(pos1.x, std::max(pos4.x, std::max(pos2.x, std::max(pos3.x, std::max(pos5.x, pos6.x)))));
		vmax.y = std::max(pos1.y, std::max(pos4.y, std::max(pos2.y, std::max(pos3.y, std::max(pos5.y, pos6.y)))));
		vmax.z = std::max(pos1.z, std::max(pos4.z, std::max(pos2.z, std::max(pos3.z, std::max(pos5.z, pos6.z)))));
		vmin.x = std::min(pos1.x, std::min(pos4.x, std::min(pos2.x, std::min(pos3.x, std::min(pos5.x, pos6.x)))));
		vmin.y = std::min(pos1.y, std::min(pos4.y, std::min(pos2.y, std::min(pos3.y, std::min(pos5.y, pos6.y)))));
		vmin.z = std::min(pos1.z, std::min(pos4.z, std::min(pos2.z, std::min(pos3.z, std::min(pos5.z, pos6.z)))));

		_frustumCenter = vmax + vmin;
		_frustumCenter = _frustumCenter * 0.5f;
		_frustumRadius = _frustumCenter.GetDistance(vmax);

		frustums._frustumLeft = Plane::WithTriangle(pos1, pos2, pos3, 1.0f);
		frustums._frustumRight = Plane::WithTriangle(pos4, pos5, pos6, -1.0f);
		frustums._frustumTop =  Plane::WithTriangle(pos1, pos2, pos5, -1.0f);
		frustums._frustumBottom = Plane::WithTriangle(pos4, pos3, pos6, 1.0f);
		frustums._frustumNear = Plane::WithPositionNormal(position + direction * _clipNear, -direction);
		frustums._frustumFar = Plane::WithPositionNormal(position + direction * _clipFar, direction);
	}

	Vector3 Camera::__ToWorld(const Vector3 &dir)
	{
		Vector3 ndcPos(dir.x, dir.y, dir.z*2.0f-1.0f);

		if(_flags & Flags::Orthogonal)
		{
			Vector4 temp = Vector4(ndcPos*0.5f);
			temp += 0.5f;
			Vector4 temp2(1.0f-temp.x, 1.0f-temp.y, 1.0f-temp.z, 0.0f);

			// I have no idea why the fourth parameter has to be 2, but translation is wrong otherwize...
			Vector4 vec(_orthoLeft, _orthoBottom, -_clipNear, 2.0f);
			vec *= temp2;
			vec += Vector4(_orthoRight, _orthoTop, -_clipFar, 2.0f)*temp;

			vec = _inverseViewMatrix * vec;
			return Vector3(vec);
		}
		else
		{
			Vector4 clipPos;
			clipPos.w = _projectionMatrix.m[14] / (ndcPos.z + _projectionMatrix.m[10]);
			clipPos = Vector4(ndcPos*clipPos.w, clipPos.w);

			Vector4 temp = _inverseProjectionMatrix * clipPos;
			temp = _inverseViewMatrix * temp;
			return Vector3(temp);
		}
	}

	// There should be a much better solution, but at least this works for now
	Vector3 Camera::ToWorld(const Vector3 &dir)
	{
		Vector3 ndcPos(dir.x, dir.y, 0.0f);
		if(_flags & Flags::Orthogonal)
		{
			Vector4 temp = Vector4(ndcPos*0.5f);
			temp += 0.5f;
			Vector4 temp2(1.0f-temp.x, 1.0f-temp.y, 0.0f, 0.0f);

			// I have no idea why the fourth parameter has to be 2, but translation is wrong otherwize...
			Vector4 vec(_orthoLeft, _orthoBottom, -dir.z, 2.0f);
			vec *= temp2;
			vec += Vector4(_orthoRight, _orthoTop, -dir.z, 2.0f)*temp;
			vec = _inverseViewMatrix * vec;
			return Vector3(vec);
		}
		else
		{
			Vector4 clipPos;
			clipPos.w = _projectionMatrix.m[14] / (ndcPos.z + _projectionMatrix.m[10]);
			clipPos = Vector4(ndcPos*clipPos.w, clipPos.w);

			Vector4 temp = _inverseProjectionMatrix * clipPos;
			temp *= -dir.z/temp.z;
			temp.w = 1.0f;
			temp = _inverseViewMatrix * temp;
			return Vector3(temp);
		}
	}

	const Rect &Camera::GetFrame()
	{
		return _frame;
	}

/*	LightManager *Camera::GetLightManager()
	{
		if(!_lightManager && _prefersLightManager)
		{
			SetLightManager(LightManager::CreateDefaultLightManager());
			_lightManager->Release(); // SetLightManager() retains the light manager, and CreateDefaultLightManager() delegates the ownership to the caller
		}

		return _lightManager;
	}*/

	const Vector3 &Camera::GetFrustumCenter()
	{
		UpdateFrustum();
		return _frustumCenter;
	}

	float Camera::GetFrustumRadius()
	{
		UpdateFrustum();
		return _frustumRadius;
	}

	bool Camera::InFrustum(const Vector3 &position, float radius)
	{
		if(_frustumCenter.GetDistance(position) > _frustumRadius + radius)
			return false;

		if(frustums._frustumLeft.GetDistance(position) > radius)
			return false;

		if(frustums._frustumRight.GetDistance(position) > radius)
			return false;

		if(frustums._frustumTop.GetDistance(position) > radius)
			return false;

		if(frustums._frustumBottom.GetDistance(position) > radius)
			return false;

		if(frustums._frustumNear.GetDistance(position) > radius)
			return false;

		if(frustums._frustumFar.GetDistance(position) > radius)
			return false;

		return true;
	}
}
