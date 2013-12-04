//
//  RNLight.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLight.h"
#include "RNWorld.h"
#include "RNCamera.h"
#include "RNResourcePool.h"
#include "RNLightManager.h"
#include "RNLogging.h"

namespace RN
{
	RNDeclareMeta(Light)
	
	Light::Light(Type lighttype) :
		_lightType(lighttype),
		_color("color", Color(1.0f), std::bind(&Light::GetColor, this), std::bind(&Light::SetColor, this, std::placeholders::_1)),
		_intensity("intensity", 10.0f, std::bind(&Light::GetIntensity, this), std::bind(&Light::SetIntensity, this, std::placeholders::_1)),
		_range("range", 10.0f, std::bind(&Light::GetRange, this), std::bind(&Light::SetRange, this, std::placeholders::_1)),
		_angle("angle", 0.5f, std::bind(&Light::GetAngle, this), std::bind(&Light::SetAngle, this, std::placeholders::_1))
	{
		_shadow = false;
		_shadowcam = nullptr;
		_lightcam  = nullptr;
		
		collisionGroup = 25;
		
		AddObservable(&_color);
		AddObservable(&_intensity);
		AddObservable(&_range);
		AddObservable(&_angle);
		ReCalculateColor();
	}
	
	Light::~Light()
	{
		if(_shadowcam)
			_shadowcam->Release();
		
		if(_lightcam)
			_lightcam->Release();
	}
	
	bool Light::IsVisibleInCamera(Camera *camera)
	{
		if(_lightType == Type::DirectionalLight)
			return true;
		
		return SceneNode::IsVisibleInCamera(camera);
	}
	
	void Light::Render(Renderer *renderer, Camera *camera)
	{
		if(!(camera->GetFlags() & Camera::FlagNoLights))
		{
			if(camera->lightManager != nullptr)
			{
				camera->lightManager->AddLight(this);
			}
		}
	}
	
	void Light::SetRange(float range)
	{
		_range = range;
		
		SetBoundingSphere(Sphere(Vector3(), range));
		SetBoundingBox(AABB(Vector3(), range), false);
	}
	
	void Light::SetColor(const Color& color)
	{
		_color = color;
		ReCalculateColor();
	}
	
	void Light::SetIntensity(float intensity)
	{
		_intensity = intensity;
		ReCalculateColor();
	}
	
	void Light::SetAngle(float angle)
	{
		_angle = angle;
	}
	
	void Light::SetLightCamera(Camera *lightCamera)
	{
		if(_lightcam)
		{
			RemoveDependency(_lightcam);
			_lightcam->Release();
		}
		
		_lightcam = lightCamera ? lightCamera->Retain() : nullptr;
		AddDependency(_lightcam);
	}
	
	void Light::RemoveShadowCameras()
	{
		_shadowcams.Enumerate<Camera>([&](Camera *camera, size_t index, bool *stop) {
			RemoveDependency(camera);
		});
		
		_shadowcams.RemoveAllObjects();
		
		if(_shadowcam)
		{
			RemoveDependency(_shadowcam);
			_shadowcam->Release();
			_shadowcam = nullptr;
		}
	}
	
	
	void Light::ActivateDirectionalShadows(bool shadow, int resolution, int splits, float distfac, float biasfac, float biasunits)
	{
		if(_lightType != Type::DirectionalLight)
			return;
		
		if(_shadow == shadow)
			return;
		
		_shadow = shadow;
		RemoveShadowCameras();
		
		if(_shadow)
		{
			_shadowSplits  = splits;
			_shadowDistFac = distfac;
			
			Texture::Parameter parameter;
			parameter.wrapMode = Texture::WrapMode::Clamp;
			parameter.filter = Texture::Filter::Linear;
			parameter.format = Texture::Format::Depth24I;
			parameter.depthCompare = true;
			parameter.maxMipMaps = 0;
			
			Texture2DArray *depthtex = new Texture2DArray(parameter);
			depthtex->SetSize(32, 32, splits);
			depthtex->Autorelease();
			
			Shader   *depthShader = ResourcePool::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyDirectionalShadowDepthShader);
			Material *depthMaterial = new Material(depthShader);
			depthMaterial->polygonOffset = true;
			depthMaterial->polygonOffsetFactor = biasfac;
			depthMaterial->polygonOffsetUnits  = biasunits;
			
			for(int i = 0; i < _shadowSplits; i++)
			{
				RenderStorage *storage = new RenderStorage(RenderStorage::BufferFormatDepth, 0, 1.0f);
				storage->SetDepthTarget(depthtex, i);
				
				Camera *tempcam = new Camera(Vector2(resolution), storage, Camera::FlagUpdateAspect | Camera::FlagUpdateStorageFrame | Camera::FlagOrthogonal | Camera::FlagHidden | Camera::FlagNoLights, 1.0f);
				tempcam->SetMaterial(depthMaterial);
				tempcam->SetLODCamera(_lightcam);
				tempcam->SetPriority(kRNShadowCameraPriority);
				tempcam->clipnear = 1.0f;

				_shadowcams.AddObject(tempcam);
				AddDependency(tempcam);
				
				tempcam->Release();
				storage->Release();
			}
		}
	}
	
	void Light::ActivatePointShadows(bool shadow, int resolution)
	{
		if(_lightType != Type::PointLight)
			return;
		
		if(_shadow == shadow)
			return;
		
		_shadow = shadow;
		RemoveShadowCameras();
		
		if(_shadow)
		{
			Texture::Parameter parameter;
			parameter.wrapMode = Texture::WrapMode::Repeat;
			parameter.filter = Texture::Filter::Nearest;
			parameter.format = Texture::Format::Depth24I;
			parameter.depthCompare = false;
			parameter.maxMipMaps = 0;
			
			Texture *depthtex = new TextureCubeMap(parameter);
			depthtex->Autorelease();
			
			Shader   *depthShader = ResourcePool::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyPointShadowDepthShader);
			Material *depthMaterial = new Material(depthShader);
			
			RenderStorage *storage = new RenderStorage(RenderStorage::BufferFormatDepth, 0, 1.0f);
			storage->SetDepthTarget(depthtex, -1);
			
			_shadowcam = new CubemapCamera(Vector2(resolution), storage, Camera::FlagUpdateAspect | Camera::FlagUpdateStorageFrame | Camera::FlagHidden | Camera::FlagNoLights, 1.0f);
			_shadowcam->Retain();
			_shadowcam->Autorelease();
			_shadowcam->SetMaterial(depthMaterial);
			_shadowcam->SetPriority(kRNShadowCameraPriority);
			_shadowcam->clipnear = 0.01f;
			_shadowcam->clipfar = _range;
			_shadowcam->fov = 90.0f;
			_shadowcam->UpdateProjection();
			_shadowcam->SetWorldRotation(Vector3(0.0f, 0.0f, 0.0f));
			
			AddDependency(_shadowcam);
			storage->Release();
		}
	}
	
	void Light::ActivateSpotShadows(bool shadow, int resolution)
	{
		if(_lightType != Type::SpotLight)
		return;
		
		if(_shadow == shadow)
		return;
		
		_shadow = shadow;
		RemoveShadowCameras();
		
		if(_shadow)
		{
			Texture::Parameter parameter;
			parameter.wrapMode = Texture::WrapMode::Repeat;
			parameter.filter = Texture::Filter::Nearest;
			parameter.format = Texture::Format::Depth24I;
			parameter.depthCompare = false;
			parameter.maxMipMaps = 0;
			
			Texture *depthtex = new TextureCubeMap(parameter);
			depthtex->Autorelease();
			
			Shader   *depthShader = ResourcePool::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyPointShadowDepthShader);
			Material *depthMaterial = new Material(depthShader);
			
			RenderStorage *storage = new RenderStorage(RenderStorage::BufferFormatDepth, 0, 1.0f);
			storage->SetDepthTarget(depthtex, -1);
			
			_shadowcam = new CubemapCamera(Vector2(resolution), storage, Camera::FlagUpdateAspect | Camera::FlagUpdateStorageFrame | Camera::FlagHidden | Camera::FlagNoLights, 1.0f);
			_shadowcam->Retain();
			_shadowcam->Autorelease();
			_shadowcam->SetMaterial(depthMaterial);
			_shadowcam->SetPriority(kRNShadowCameraPriority);
			_shadowcam->clipnear = 0.01f;
			_shadowcam->clipfar = _range;
			_shadowcam->fov = 90.0f;
			_shadowcam->UpdateProjection();
			_shadowcam->SetWorldRotation(Vector3(0.0f, 0.0f, 0.0f));
			
			AddDependency(_shadowcam);
			storage->Release();
		}
	}
	
	void Light::Update(float delta)
	{
		SceneNode::Update(delta);
		
		if(_lightType == Type::DirectionalLight)
		{
			Log::Logger::GetSharedInstance()->Log(Log::Level::Info, "Light::Update()");
			
			if(_shadow && _lightcam)
			{
				float near = _lightcam->clipnear;
				float far;
				
				if(_shadowcam)
					_shadowcam->SetWorldRotation(GetWorldRotation());
				
				_shadowmats.clear();
				
				for(int i = 0; i < _shadowSplits; i++)
				{
					float linear = _lightcam->clipnear+(_lightcam->clipfar-_lightcam->clipnear)*(i+1.0f)/float(_shadowSplits);
					float log = _lightcam->clipnear*powf(_lightcam->clipfar/_lightcam->clipnear, (i+1.0f)/float(_shadowSplits));
					far = linear*_shadowDistFac+log*(1.0f-_shadowDistFac);
					
					if(_shadowcam)
					{
						_shadowmats.push_back(std::move(_shadowcam->MakeShadowSplit(_lightcam, this, near, far)));
					}
					else
					{
						Camera *tempcam = _shadowcams.GetObjectAtIndex<Camera>(i);
						tempcam->SetWorldRotation(GetWorldRotation());
						
						_shadowmats.push_back(std::move(tempcam->MakeShadowSplit(_lightcam, this, near, far)));
					}
					
					near = far;
				}
				
				if(_shadowcam)
				{
					_shadowcam->MakeShadowSplit(_lightcam, this, _lightcam->clipnear, _lightcam->clipfar);
				}
			}
		}
		else
		{
			if(_shadow && _shadowcam)
			{
				_shadowcam->SetWorldPosition(GetWorldPosition());
				_shadowcam->clipfar = _range;
				_shadowcam->UpdateProjection();
			}
		}
	}

	void Light::ReCalculateColor()
	{
		_resultColor = Vector3(_color->r, _color->g, _color->b);
		_resultColor *= (float)_intensity;
	}
}
