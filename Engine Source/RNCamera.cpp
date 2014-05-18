//
//  RNCamera.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCamera.h"
#include "RNThread.h"
#include "RNKernel.h"
#include "RNWorld.h"
#include "RNLight.h"
#include "RNLightManager.h"
#include "RNResourceCoordinator.h"
#include "RNOpenGLQueue.h"
#include "RNLogging.h"
#include "RNMessage.h"
#include "RNWindow.h"

namespace RN
{
	RNDefineMeta(Camera, SceneNode)
	RNDefineMeta(PostProcessingPipeline, Object)
	
	RenderStage::RenderStage(Camera *camera, Camera *connection, Mode mode)
	{
		_camera = camera->Retain();
		_connection = 0;
		_mode = mode;
		
		Connect(connection);
		InsertCamera(camera);
	}
	
	RenderStage::RenderStage(const RenderStage& other)
	{
		_camera = other._camera->Retain();
		_connection = 0;
		_mode = other._mode;
		
		Connect(other._connection);
		InsertCamera(_camera);
	}
	
	RenderStage::~RenderStage()
	{
		RemoveCamera(_camera);
		RemoveCamera(_connection);
		
		_camera->Release();
		
		if(_connection)
			_connection->Release();
	}
	
	void RenderStage::Connect(class Camera *other)
	{
		if(_connection)
		{
			RemoveCamera(_connection);
			_connection->Release();
		}
		
		_connection = other ? other->Retain() : 0;
		InsertCamera(_connection);
	}
	
	void RenderStage::InsertCamera(Camera *camera)
	{
		if(!camera || _mode > Mode::ReUseCamera)
			return;
		
		if((camera->_stageCount ++) == 0)
			World::GetActiveWorld()->RemoveSceneNode(camera);
	}
	
	void RenderStage::RemoveCamera(Camera *camera)
	{
		if(!camera || _mode > Mode::ReUseCamera)
			return;
		
		if((-- camera->_stageCount) == 0)
			World::GetActiveWorld()->AddSceneNode(camera);
	}
	
	
	
	
	PostProcessingPipeline::PostProcessingPipeline(const std::string& name, int32 priority) :
		_name(name),
		_priority(priority),
		host(nullptr)
	{}
	
	PostProcessingPipeline::~PostProcessingPipeline()
	{}
	
	void PostProcessingPipeline::Initialize()
	{}
	
	
	RenderStage *PostProcessingPipeline::AddStage(Camera *camera, RenderStage::Mode mode)
	{
		Camera *previous = nullptr;
		
		if(stages.size() > 0)
		{
			RenderStage& stage = stages[stages.size() - 1];
			previous = stage.GetCamera();
		}
		
		return AddStage(camera, previous, mode);
	}
	
	RenderStage *PostProcessingPipeline::AddStage(Camera *camera, Camera *connection, RenderStage::Mode mode)
	{
		stages.emplace_back(camera, connection, mode);
		return &stages[stages.size() - 1];
	}
	
	void PostProcessingPipeline::PushUpdate(Camera *source, float delta)
	{
		for(auto i = stages.begin(); i != stages.end(); i ++)
		{
			Camera *camera = i->GetCamera();
			if(camera == source)
				continue;
			
			camera->Update(delta);
		}
	}
	
	void PostProcessingPipeline::PostUpdate(Camera *source, const Vector3& position, const Quaternion& rotation, const Rect& frame)
	{
		for(auto i = stages.begin(); i != stages.end(); i ++)
		{
			Camera *camera = i->GetCamera();
			if(camera  == source)
				continue;
			
			if(camera->_flags & Camera::Flags::InheritFrame)
				camera->SetFrame(frame);
			
			if(camera->_flags & Camera::Flags::InheritPosition)
			{
				camera->SetWorldPosition(position);
				camera->SetWorldRotation(rotation);
			}
			
			camera->PostUpdate();
		}
	}
	
	void PostProcessingPipeline::PushProjectionUpdate(Camera *source)
	{
		for(auto i = stages.begin(); i != stages.end(); i ++)
		{
			Camera *stage = i->GetCamera();
			if(stage == source)
				continue;
			
			if(stage->_flags & Camera::Flags::InheritProjection)
			{
				stage->SetAspectRatio(source->GetAspectRatio());
				stage->SetFOV(source->GetFOV());
				
				stage->SetClipFar(source->GetClipFar());
				stage->SetClipNear(source->GetClipNear());
				
				stage->UpdateProjection();
			}
		}
	}
	
	
	Camera::Camera() :
		Camera(Vector2(0.0f))
	{}
	
	Camera::Camera(const Vector2& size) :
		Camera(size, Texture::Format::RGB16F)
	{}


	Camera::Camera(const Vector2& size, Texture *target) :
		Camera(size, target, Flags::Defaults)
	{}

	Camera::Camera(const Vector2& size, Texture *target, Flags flags) :
		Camera(size, target, flags, RenderStorage::BufferFormatComplete)
	{}

	Camera::Camera(const Vector2& size, Texture::Format targetFormat) :
		Camera(size, targetFormat, Flags::Defaults)
	{}

	Camera::Camera(const Vector2& size, Texture::Format targetFormat, Flags flags) :
		Camera(size, targetFormat, flags, RenderStorage::BufferFormatComplete)
	{}


	Camera::Camera(const Vector2& size, Texture *target, Flags flags, RenderStorage::BufferFormat format) :
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

	Camera::Camera(const Vector2& size, Texture::Format targetFormat, Flags flags, RenderStorage::BufferFormat format, float scaleFactor) :
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

	Camera::Camera(const Vector2& size, RenderStorage *storage, Flags flags, float scaleFactor) :
		_frame(Vector2(0.0f, 0.0f), size),
		_scaleFactor(scaleFactor),
		_storage(nullptr),
		_lightManager(nullptr)
	{
		SetFlags(flags);
		SetRenderStorage(storage);
		Initialize();
	}

	Camera::~Camera()
	{
		SafeRelease(_storage);
		SafeRelease(_sky);
		SafeRelease(_material);
		SafeRelease(_blitShader);
		
		for(PostProcessingPipeline *pipeline : _PPPipelines)
			pipeline->Release();
		
		if(_lightManager)
		{
			_lightManager->camera = nullptr;
			_lightManager->Release();
		}
		
		MessageCenter::GetSharedInstance()->RemoveObserver(this);
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
		
		_fixedScaleFactor = (_scaleFactor > 0.0f);
		_scaleFactor      = _fixedScaleFactor ? _scaleFactor : Kernel::GetSharedInstance()->GetActiveScaleFactor();

		_material   = nullptr;
		_stageCount = 0;

		_sky = nullptr;
		
		_priority  = 0;
		_lodCamera = nullptr;
		
		_blitShader = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyDrawFramebufferShader, nullptr)->Retain();
		_blitMode   = BlitMode::Stretched;
		
		_clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
		_colorMask = ColorMask::Red | ColorMask::Green | ColorMask::Blue | ColorMask::Alpha;
		
		_renderGroups = RenderGroups::Group0;

		if(_flags & Flags::UpdateStorageFrame)
			_storage->SetSize(_frame.GetSize());
		
		Update(0.0f);
		UpdateProjection();
		UpdateFrustum();
		
		_prefersLightManager = true;
		
		MessageCenter::GetSharedInstance()->AddObserver(kRNWindowScaleFactorChanged, [this](Message *message) {
			_scaleFactor = Kernel::GetSharedInstance()->GetActiveScaleFactor();
		}, this);
	}

	void Camera::PrepareForRendering(Renderer *renderer)
	{
		UpdateProjection();
		_storage->BindAndUpdateBuffer();
		
		Rect rect = std::move(GetRenderingFrame());
		
		float x = ceilf(rect.x * _scaleFactor);
		float y = ceilf(rect.y * _scaleFactor);
		
		float width  = ceilf(rect.width * _scaleFactor);
		float height = ceilf(rect.height * _scaleFactor);

		if(_clearMask > 0)
		{
			OpenGLQueue::GetSharedInstance()->SubmitCommand([=] {
				gl::ClearDepth(1.0f);
				gl::ClearStencil(0);
				gl::ClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
				
				gl::Scissor(x, y, width, height);
				gl::Clear(_clearMask);
				
				renderer->SetScissorEnabled(true);
			});
		}
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([=] {
			gl::ColorMask((_colorMask & ColorMask::Red), (_colorMask & ColorMask::Green), (_colorMask & ColorMask::Blue), (_colorMask & ColorMask::Alpha));
			gl::Viewport(x, y, width, height);
		});
	}

	// Setter
	void Camera::SetFrame(const Rect& frame)
	{
		if(_frame != frame)
		{
			_frame = std::move(frame.GetIntegral());

			if(_flags & Flags::UpdateStorageFrame)
				_storage->SetSize(frame.GetSize());

			_dirtyProjection = true;
		}
	}

	void Camera::SetRenderingFrame(const Rect& frame)
	{
		_renderingFrame = std::move(frame.GetIntegral());
	}
	
	void Camera::SetClearColor(const Color& clearColor)
	{
		_clearColor = clearColor;
	}
	
	void Camera::SetFlags(Flags flags)
	{
		_flags = flags;
	}

	void Camera::SetMaterial(Material *material)
	{
		SafeRelease(_material);
		_material = SafeRetain(material);
	}

	void Camera::SetRenderStorage(RenderStorage *storage)
	{
		RN_ASSERT(storage, "Render storage mustn't be NULL!");

		SafeRelease(_storage);
		_storage = SafeRetain(storage);

		if(_flags & Flags::UpdateStorageFrame)
			_storage->SetSize(_frame.GetSize());
	}
	
	void Camera::SetClearMask(ClearMask mask)
	{
		_clearMask = 0;
		
		if(mask & ClearMask::Color)
			_clearMask |= GL_COLOR_BUFFER_BIT;
		
		if(mask & ClearMask::Depth)
			_clearMask |= GL_DEPTH_BUFFER_BIT;
		
		if(mask & ClearMask::Stencil)
			_clearMask |= GL_STENCIL_BUFFER_BIT;
	}
	
	void Camera::SetColorMask(ColorMask mask)
	{
		_colorMask = mask;
	}
	
	void Camera::SetSky(Model *sky)
	{
		SafeRelease(_sky);
		_sky = SafeRetain(sky);
	}
	
	void Camera::SetLODCamera(Camera *camera)
	{
		_lodCamera = camera;
	}
	
	void Camera::SetPriority(int32 priority)
	{
		_priority = priority;
	}
	
	void Camera::SetBlitShader(Shader *shader)
	{
		RN_ASSERT(shader, "Shader musn't be NULL!");
		
		_blitShader->Release();
		_blitShader = shader->Retain();
	}
	
	void Camera::SetBlitMode(BlitMode mode)
	{
		_blitMode = mode;
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
		
		if(_flags & Flags::UpdateAspect)
			RNDebug("SetAspectRatio() called, but auto aspect ratio updating is set (will invaldiate set ratio on next frame update)");
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
	
	void Camera::SetLightManager(LightManager *lightManager)
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
	}
	void Camera::SetRenderGroups(RenderGroups groups)
	{
		_renderGroups = groups;
	}
	void Camera::SetOrthogonalFrustum(float top, float bottom, float left, float right)
	{
		_orthoLeft   = left;
		_orthoRight  = right;
		_orthoTop    = top;
		_orthoBottom = bottom;
		
		_dirtyProjection = true;
		
		if(!(_flags & Flags::Orthogonal))
			RNDebug("SetOrthogonalFrustum() called, but the camera is not an orthogonal camera");
	}
	
	void Camera::SetProjectionMatrix(const Matrix &projectionMatrix)
	{
		_projectionMatrix = projectionMatrix;
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
	PostProcessingPipeline *Camera::AddPostProcessingPipeline(const std::string& name, int32 priority)
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
	
	PostProcessingPipeline *Camera::GetPostProcessingPipeline(const std::string& name)
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
	}
	
	Matrix Camera::MakeShadowSplit(Camera *camera, Light *light, float near, float far)
	{
		//Get camera frustums extends to be covered by the split
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
	}
	
	// Helper
	void Camera::Update(float delta)
	{
		SceneNode::Update(delta);
		
		UpdateProjection();
		UpdateFrustum();
		
		for(auto i=_PPPipelines.begin(); i!=_PPPipelines.end(); i++)
		{
			PostProcessingPipeline *pipeline = *i;
			pipeline->PushUpdate(this, delta);
		}
	}
	
	void Camera::UpdateEditMode(float delta)
	{
		SceneNode::UpdateEditMode(delta);
		
		UpdateProjection();
		UpdateFrustum();
		
		for(auto i=_PPPipelines.begin(); i!=_PPPipelines.end(); i++)
		{
			PostProcessingPipeline *pipeline = *i;
			pipeline->PushUpdate(this, delta);
		}
	}
	
	void Camera::PostUpdate()
	{
		_inverseViewMatrix = GetWorldTransform();
		_viewMatrix = _inverseViewMatrix.GetInverse();
		
		if(_flags & Flags::Fullscreen)
		{
			Vector2 size = Window::GetSharedInstance()->GetSize();
			SetFrame(Rect(Vector2(), size));
		}

		for(auto i=_PPPipelines.begin(); i!=_PPPipelines.end(); i++)
		{
			PostProcessingPipeline *pipeline = *i;
			pipeline->PostUpdate(this, GetWorldPosition(), GetWorldRotation(), _frame);
		}
	}

	void Camera::UpdateProjection()
	{
		if(!_dirtyProjection)
			return;
		
		_dirtyProjection = false;
		
		if(_flags & Flags::Orthogonal)
		{
			_projectionMatrix = Matrix::WithProjectionOrthogonal(_orthoLeft, _orthoRight, _orthoBottom, _orthoTop, _clipNear, _clipFar);
			return;
		}
		
		if(_flags & Flags::UpdateAspect)
			_aspect = _frame.width / _frame.height;

		_projectionMatrix = Matrix::WithProjectionPerspective(_fov, _aspect, _clipNear, _clipFar);
		_inverseProjectionMatrix = _projectionMatrix.GetInverse();

		for(auto i=_PPPipelines.begin(); i!=_PPPipelines.end(); i++)
		{
			PostProcessingPipeline *pipeline = *i;
			pipeline->PushProjectionUpdate(this);
		}
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
		
		const Vector3& position = GetWorldPosition();
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
		
		frustrums._frustumLeft = Plane::WithTriangle(pos1, pos2, pos3, 1.0f);
		frustrums._frustumRight = Plane::WithTriangle(pos4, pos5, pos6, -1.0f);
		frustrums._frustumTop =  Plane::WithTriangle(pos1, pos2, pos5, -1.0f);
		frustrums._frustumBottom = Plane::WithTriangle(pos4, pos3, pos6, 1.0f);
		frustrums._frustumNear = Plane::WithPositionNormal(position + direction * _clipNear, -direction);
		frustrums._frustumFar = Plane::WithPositionNormal(position + direction * _clipFar, direction);
	}

	Vector3 Camera::__ToWorld(const Vector3& dir)
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
	Vector3 Camera::ToWorld(const Vector3& dir)
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
	
	const Rect& Camera::GetFrame()
	{
		if(_flags & Flags::Fullscreen)
		{
			Vector2 size = Window::GetSharedInstance()->GetSize();
			SetFrame(Rect(Vector2(), size));
		}
		
		return _frame;
	}
	
	Rect Camera::GetRenderingFrame()
	{
		if(_renderingFrame.x + _renderingFrame.y + _renderingFrame.width + _renderingFrame.height <= k::EpsilonFloat)
			return GetFrame();
		
		return _renderingFrame;
	}
	
	LightManager *Camera::GetLightManager()
	{
		if(!_lightManager && _prefersLightManager)
		{
			SetLightManager(LightManager::CreateDefaultLightManager());
			_lightManager->Release(); // SetLightManager() retains the light manager, and CreateDefaultLightManager() delegates the ownership to the caller
		}
		
		return _lightManager;
	}
	
	const Vector3& Camera::GetFrustumCenter()
	{
		UpdateFrustum();
		return _frustumCenter;
	}
	
	float Camera::GetFrustumRadius()
	{
		UpdateFrustum();
		return _frustumRadius;
	}
	

	bool Camera::InFrustum(const Vector3& position, float radius)
	{
		UpdateFrustum();
		
		if(_frustumCenter.GetDistance(position) > _frustumRadius + radius)
			return false;

		if(frustrums._frustumLeft.GetDistance(position) > radius)
			return false;

		if(frustrums._frustumRight.GetDistance(position) > radius)
			return false;

		if(frustrums._frustumTop.GetDistance(position) > radius)
			return false;

		if(frustrums._frustumBottom.GetDistance(position) > radius)
			return false;
		
		if(frustrums._frustumNear.GetDistance(position) > radius)
			return false;
		
		if(frustrums._frustumFar.GetDistance(position) > radius)
			return false;

		return true;
	}
	
	bool Camera::InFrustum(const Sphere& sphere)
	{
		UpdateFrustum();
		return InFrustum(sphere.position+sphere.offset, sphere.radius);
	}
	
	bool Camera::InFrustum(const AABB& aabb)
	{
		UpdateFrustum();
		
/*		Plane *planes = &_frustumLeft;
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
		}*/
		
		return true;
	}
	
	bool Camera::IsVisibleInCamera(Camera *camera)
	{
		return true;
	}
	
	Hit Camera::CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode)
	{
		Hit hit;
		return hit;
	}
}
