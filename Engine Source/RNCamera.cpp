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
#include "RNResourcePool.h"

namespace RN
{
	RNDeclareMeta(Camera)
	
	
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
			World::GetSharedInstance()->RemoveSceneNode(camera);
	}
	
	void RenderStage::RemoveCamera(Camera *camera)
	{
		if(!camera || _mode > Mode::ReUseCamera)
			return;
		
		if((-- camera->_stageCount) == 0)
			World::GetSharedInstance()->AddSceneNode(camera);
	}
	
	
	
	
	PostProcessingPipeline::PostProcessingPipeline(const std::string& name) :
		_name(name)
	{
		host = 0;
	}
	
	PostProcessingPipeline::~PostProcessingPipeline()
	{
	}
	
	void PostProcessingPipeline::Initialize()
	{
	}
	
	
	RenderStage *PostProcessingPipeline::AddStage(Camera *camera, RenderStage::Mode mode)
	{
		Camera *previous = 0;
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
		for(auto i=stages.begin(); i!=stages.end(); i++)
		{
			Camera *camera = i->GetCamera();
			if(camera == source)
				continue;
			
			camera->Update(delta);
		}
	}
	
	void PostProcessingPipeline::PostUpdate(Camera *source, const Vector3& position, const Quaternion& rotation, const Rect& frame)
	{
		for(auto i=stages.begin(); i!=stages.end(); i++)
		{
			Camera *camera = i->GetCamera();
			if(camera  == source)
				continue;
			
			if(camera->_flags & Camera::FlagInheritFrame)
				camera->SetFrame(frame);
			
			if(camera->_flags & Camera::FlagInheritPosition)
			{
				camera->SetWorldPosition(position);
				camera->SetWorldRotation(rotation);
			}
			
			camera->PostUpdate();
		}
	}
	
	void PostProcessingPipeline::PushProjectionUpdate(Camera *source)
	{
		for(auto i=stages.begin(); i!=stages.end(); i++)
		{
			Camera *stage = i->GetCamera();
			if(stage == source)
				continue;
			
			if(stage->_flags & Camera::FlagInheritProjection)
			{
				stage->aspect = source->aspect;
				stage->fov    = source->fov;
				
				stage->clipfar  = source->clipfar;
				stage->clipnear = source->clipnear;
				
				stage->UpdateProjection();
			}
		}
	}
	
	
	DownsamplePostProcessingPipeline::DownsamplePostProcessingPipeline(const std::string& name, Camera *camera, Texture *texture, Shader *firstShader, Shader *shader, TextureParameter::Format format) :
		PostProcessingPipeline(name)
	{
		TextureParameter parameter;
		parameter.format = format;
		
		_lastTarget = new Texture(parameter);
		_format = format;
		_level = 0;
		
		_texture = texture ? texture->Retain() : 0;
		_camera = camera->Retain();
		_firstShader = firstShader ? firstShader->Retain() : shader->Retain();
		_shader = shader->Retain();
	}
	
	DownsamplePostProcessingPipeline::~DownsamplePostProcessingPipeline()
	{
		_firstShader->Release();
		_shader->Release();
		
		_camera->Release();
		_texture->Release();
		_lastTarget->Release();
	}
	
	void DownsamplePostProcessingPipeline::Initialize()
	{
		_level = 0;
		_frame = Rect();
		
		PushUpdate(nullptr, 0.0f);
	}
	
	void DownsamplePostProcessingPipeline::PushUpdate(Camera *source, float delta)
	{
		bool needsUpdate = false;
		bool needsRecreation = false;
		
		if(_frame != host->GetFrame())
		{
			_frame = host->GetFrame();
			needsUpdate = true;
		}
		
		int level = Kernel::GetSharedInstance()->GetActiveScaleFactor() + log2(_camera->GetLightTiles().x) - 1;
		if(level != _level)
		{
			_level = level;
			needsRecreation = true;
		}
		
		
		if(needsRecreation)
		{
			RecreateStages();
		}
		else if(needsUpdate)
		{
			UpdateStages();
		}
		
		PostProcessingPipeline::PushUpdate(source, delta);
	}
	
	void DownsamplePostProcessingPipeline::UpdateStages()
	{
		int factor = 1;
		
		for(auto i=stages.begin(); i!=stages.end(); i++)
		{
			factor <<= 1;
			i->GetCamera()->SetFrame(Rect(Vector2(0.0f), Vector2(_frame.Size().x / factor, _frame.Size().y / factor)));
		}
	}
	
	void DownsamplePostProcessingPipeline::RecreateStages()
	{
		stages.clear();
		uint32 flags = Camera::FlagUpdateStorageFrame | Camera::FlagInheritProjection;
		
		for(int i=0; i<_level; i++)
		{
			Material *temp = new Material((i == 0) ? _firstShader : _shader);
			temp->AddTexture(_texture);
			
			Camera *camera = new Camera(Vector2(), _format, flags, RenderStorage::BufferFormatColor);
			camera->SetMaterial(temp);
			
			if(i == _level - 1)
				camera->GetStorage()->SetRenderTarget(_lastTarget);
			
			AddStage(camera, RenderStage::Mode::ReUsePreviousStage);
			
			camera->Release();
			temp->Release();
		}
		
		UpdateStages();
	}
	
	
	
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

	Camera::Camera(const Vector2& size, TextureParameter::Format targetFormat, Flags flags, RenderStorage::BufferFormat format, float scaleFactor) :
		_frame(Vector2(0.0f, 0.0f), size),
		_flags(flags),
		_scaleFactor(scaleFactor)
	{
		_storage = 0;

		RenderStorage *storage = new RenderStorage(format, 0, scaleFactor);
		storage->AddRenderTarget(targetFormat);
		
		SetRenderStorage(storage);
		Initialize();
	}

	Camera::Camera(const Vector2& size, RenderStorage *storage, Flags flags, float scaleFactor) :
		_frame(Vector2(0.0f, 0.0f), size),
		_flags(flags),
		_scaleFactor(scaleFactor)
	{
		_storage = 0;

		SetRenderStorage(storage);
		Initialize();
	}

	Camera::~Camera()
	{
		if(_storage)
			_storage->Release();
		
		if(_skycube)
			_skycube->Release();
		
		if(_material)
			_material->Release();
		
		if(_depthTiles)
			_depthTiles->Release();
		
		_blitShader->Release();
		
		for(auto i=_PPPipelines.begin(); i!=_PPPipelines.end(); i++)
		{
			PostProcessingPipeline *pipeline = *i;
			delete pipeline;
		}
		
		MessageCenter::GetSharedInstance()->RemoveObserver(this);
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
		
		usefog = true;
		fognear = 100.0f;
		fogfar = 500.0f;
		
		useclipplane = false;
		clipplane = Vector4(0.0f, 1.0f, 0.0f, 0.0f);

		_clearColor  = Color(0.193f, 0.435f, 0.753f, 1.0f);
		
		_fixedScaleFactor = (_scaleFactor > 0.0f);
		_scaleFactor = _fixedScaleFactor ? _scaleFactor : Kernel::GetSharedInstance()->GetActiveScaleFactor();

		_material = 0;
		_stageCount = 0;

		_lightTiles = Vector2(32, 32);
		_depthTiles = 0;
		_skycube = 0;
		
		_depthArray = 0;
		_depthSize  = 0;
		_depthFrame = 0;
		
		_maxLights = 500;
		_priority  = 0;
		
		_allowDepthWrite = true;
		_lodCamera = 0;
		_blend = false;
		
		_blitShader = ResourcePool::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyDrawFramebufferShader)->Retain();
		_blitMode   = BlitMode::Stretched;
		
		_clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
		_colorMask = ColorFlagRed | ColorFlagGreen | ColorFlagBlue | ColorFlagAlpha;
		
		renderGroup = RenderGroup0;

		if(_flags & FlagUpdateStorageFrame)
			_storage->SetSize(_frame.Size());
		
		Update(0.0f);
		UpdateProjection();
		UpdateFrustum();
		
		MessageCenter::GetSharedInstance()->AddObserver(kRNWindowScaleFactorChanged, [this](Message *message) {
			_scaleFactor = Kernel::GetSharedInstance()->GetActiveScaleFactor();
		}, this);
	}



	void Camera::Bind()
	{
		_storage->Bind();
	}

	void Camera::Unbind()
	{
		_storage->Unbind();
	}

	void Camera::PrepareForRendering(Renderer *renderer)
	{
		_storage->UpdateBuffer();
		
		Rect rect = std::move(GetRenderingFrame());
		
		float x = ceilf(rect.x * _scaleFactor);
		float y = ceilf(rect.y * _scaleFactor);
		
		float width  = ceilf(rect.width * _scaleFactor);
		float height = ceilf(rect.height * _scaleFactor);

		if(!(_flags & FlagNoClear))
		{
			Context *context = Context::GetActiveContext();
			
			context->SetDepthClear(1.0f);
			context->SetStencilClear(0);
			context->SetClearColor(_clearColor);
			
			renderer->SetScissorEnabled(true);
			glScissor(x, y, width, height);
			
			glClear(_clearMask);
		}
		
		glColorMask((_colorMask & ColorFlagRed), (_colorMask & ColorFlagGreen), (_colorMask & ColorFlagBlue), (_colorMask & ColorFlagAlpha));
		glViewport(x, y, width, height);
	}

	// Setter
	void Camera::SetFrame(const Rect& frame)
	{
		if(_frame != frame)
		{
			_frame = std::move(frame.Integral());

			if(_flags & FlagUpdateStorageFrame)
				_storage->SetSize(frame.Size());

			UpdateProjection();
		}
	}

	void Camera::SetRenderingFrame(const Rect& frame)
	{
		_renderingFrame = std::move(frame.Integral());
	}
	
	void Camera::SetClearColor(const Color& clearColor)
	{
		_clearColor = clearColor;
	}
	
	void Camera::SetFlags(Flags flags)
	{
		_flags = flags;
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
			_storage->SetSize(_frame.Size());
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
	
	void Camera::SetMaxLightsPerTile(size_t lights)
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
	
	void Camera::SetPriority(int32 priority)
	{
		_priority = priority;
	}
	
	void Camera::SetUseBlending(bool useBlending)
	{
		_blend = useBlending;
	}
	
	void Camera::SetDrawFramebufferShader(Shader *shader)
	{
		RN_ASSERT(shader, "Shader musn't be NULL!");
		
		_blitShader->Release();
		_blitShader = shader->Retain();
	}
	
	void Camera::SetBlitMode(BlitMode mode)
	{
		_blitMode = mode;
	}
	
	// Post Processing
	
	PostProcessingPipeline *Camera::AddPostProcessingPipeline(const std::string& name)
	{
		PostProcessingPipeline *pipeline = new PostProcessingPipeline(name);
		try
		{
			AttachPostProcessingPipeline(pipeline);
		}
		catch(Exception e)
		{
			delete pipeline;
			throw e;
		}
	
		return pipeline;
	}
	
	PostProcessingPipeline *Camera::PostProcessingPipelineWithName(const std::string& name)
	{
		auto iterator = _namedPPPipelines.find(name);
		return (iterator != _namedPPPipelines.end()) ? iterator->second : 0;
	}
	
	void Camera::AttachPostProcessingPipeline(PostProcessingPipeline *pipeline)
	{
		if(PostProcessingPipelineWithName(pipeline->_name) || pipeline->host)
			throw Exception(Exception::Type::InvalidArgumentException, "A pipeline with this name already exists, or the pipeline is already associated with a camera!");
		
		_PPPipelines.push_back(pipeline);
		_namedPPPipelines.insert(std::map<std::string, PostProcessingPipeline *>::value_type(pipeline->_name, pipeline));
		
		pipeline->host = this;
		pipeline->Initialize();
	}
	
	void Camera::RemovePostProcessingPipeline(PostProcessingPipeline *pipeline)
	{
		for(auto i=_PPPipelines.begin(); i!=_PPPipelines.end(); i++)
		{
			if(*i == pipeline)
			{
				_PPPipelines.erase(i);
				_namedPPPipelines.erase(pipeline->_name);
				
				delete pipeline;
				break;
			}
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
		
		Matrix rot = light->GetWorldRotation().GetRotationMatrix();
		pos = rot.GetInverse()*pos;
		
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
		
		Matrix projview = projectionMatrix*GetWorldTransform().GetInverse();
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
		SceneNode::Update(delta);
		
		for(auto i=_PPPipelines.begin(); i!=_PPPipelines.end(); i++)
		{
			PostProcessingPipeline *pipeline = *i;
			pipeline->PushUpdate(this, delta);
		}
	}
	
	void Camera::PostUpdate()
	{
		UpdateFrustum();
		
		inverseViewMatrix = GetWorldTransform();
		viewMatrix = inverseViewMatrix.GetInverse();
		
		if(_flags & FlagFullscreen)
		{
			Rect frame = Kernel::GetSharedInstance()->GetWindow()->GetFrame();
			SetFrame(frame);
		}

		for(auto i=_PPPipelines.begin(); i!=_PPPipelines.end(); i++)
		{
			PostProcessingPipeline *pipeline = *i;
			pipeline->PostUpdate(this, GetWorldPosition(), GetWorldRotation(), _frame);
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
		inverseProjectionMatrix = projectionMatrix.GetInverse();

		for(auto i=_PPPipelines.begin(); i!=_PPPipelines.end(); i++)
		{
			PostProcessingPipeline *pipeline = *i;
			pipeline->PushProjectionUpdate(this);
		}
	}

	Vector3 Camera::ToWorld(const Vector3& dir)
	{
		Vector3 ndcPos(dir.x, dir.y, dir.z*2.0f-1.0f);
		if(_flags & FlagOrthogonal)
		{
			Vector4 temp = Vector4(ndcPos*0.5f);
			temp += 0.5f;
			Vector4 temp2(1.0f-temp.x, 1.0f-temp.y, 1.0f-temp.z, 0.0f);
			Vector4 vec = Vector4(ortholeft, orthobottom, -clipnear, 1.0f)*temp2;
			vec += Vector4(orthoright, orthotop, -clipfar, 1.0f)*temp;
			
			vec = inverseViewMatrix.Transform(vec);
			return Vector3(vec);
		}
		else
		{
			Vector4 clipPos;
			clipPos.w = projectionMatrix.m[14]/(ndcPos.z+projectionMatrix.m[10]);
			clipPos = Vector4(ndcPos*clipPos.w, clipPos.w);
			
			Vector4 temp = inverseProjectionMatrix.Transform(clipPos);
			temp = inverseViewMatrix.Transform(temp);
			return Vector3(temp);
		}
	}
	
	//There should be a much better solution, but at least this works for now
	Vector3 Camera::ToWorldZ(const Vector3& dir)
	{
		Vector3 ndcPos(dir.x, dir.y, 0.0f);
		if(_flags & FlagOrthogonal)
		{
			Vector4 temp = Vector4(ndcPos*0.5f);
			temp += 0.5f;
			Vector4 temp2(1.0f-temp.x, 1.0f-temp.y, 0.0f, 0.0f);
			Vector4 vec = Vector4(ortholeft, orthobottom, -dir.z, 1.0f)*temp2;
			vec += Vector4(orthoright, orthotop, -dir.z, 1.0f)*temp;
			vec = inverseViewMatrix.Transform(vec);
			return Vector3(vec);
		}
		else
		{
			Vector4 clipPos;
			clipPos.w = projectionMatrix.m[14]/(ndcPos.z+projectionMatrix.m[10]);
			clipPos = Vector4(ndcPos*clipPos.w, clipPos.w);
			
			Vector4 temp = inverseProjectionMatrix.Transform(clipPos);
			temp *= -dir.z/temp.z;
			temp.w = 1.0f;
			temp = inverseViewMatrix.Transform(temp);
			return Vector3(temp);
		}
	}
	
	const Rect& Camera::GetFrame()
	{
		if(_flags & FlagFullscreen)
		{
			Rect frame = Kernel::GetSharedInstance()->GetWindow()->GetFrame();
			SetFrame(frame);
		}
		
		return _frame;
	}
	
	Rect Camera::GetRenderingFrame()
	{
		if(_renderingFrame.x + _renderingFrame.y + _renderingFrame.width + _renderingFrame.height <= k::EpsilonFloat)
			return GetFrame();
		
		return _renderingFrame;
	}
	
	float *Camera::GetDepthArray()
	{
		if(!_depthTiles)
			return 0;
		
		if(Kernel::GetSharedInstance()->GetCurrentFrame() == _depthFrame)
			return _depthArray;
		
		int width  = (int)_depthTiles->GetWidth();
		int height = (int)_depthTiles->GetHeight();
		
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
		
		_depthFrame = Kernel::GetSharedInstance()->GetCurrentFrame();
		return _depthArray;
	}

	void Camera::UpdateFrustum()
	{
		Vector3 pos2 = ToWorld(Vector3(-1.0f, 1.0f, 1.0));
		Vector3 pos3 = ToWorld(Vector3(-1.0f, -1.0f, 1.0));
		Vector3 pos5 = ToWorld(Vector3(1.0f, 1.0f, 1.0));
		Vector3 pos6 = ToWorld(Vector3(1.0f, -1.0f, 1.0));
		
		const Vector3& position = GetWorldPosition();
		Vector3 direction = GetWorldRotation().RotateVector(Vector3(0.0, 0.0, -1.0));

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
	}

	bool Camera::InFrustum(const Vector3& position, float radius)
	{
		if(_frustumCenter.Distance(position) > _frustumRadius + radius)
			return false;

		if(_frustumLeft.GetDistance(position) > radius)
			return false;

		if(_frustumRight.GetDistance(position) > radius)
			return false;

		if(_frustumTop.GetDistance(position) > radius)
			return false;

		if(_frustumBottom.GetDistance(position) > radius)
			return false;
		
		if(_frustumNear.GetDistance(position) > radius)
			return false;
		
		if(_frustumFar.GetDistance(position) > radius)
			return false;

		return true;
	}
	
	bool Camera::InFrustum(const Sphere& sphere)
	{
		return InFrustum(sphere.position, sphere.radius);
	}
	
	bool Camera::InFrustum(const AABB& aabb)
	{
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
}
