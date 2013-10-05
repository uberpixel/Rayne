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
		SceneNode::Render(renderer, camera);
		renderer->RenderLight(this);
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
			_lightcam->Release();
		
		_lightcam = lightCamera ? lightCamera->Retain() : nullptr;
	}
	
	void Light::ActivateDirectionalShadows(bool shadow, int resolution, int splits, float distfac, float biasfac, float biasunits)
	{
		if(_lightType != Type::DirectionalLight)
			return;
		
		if(_shadow == shadow)
			return;
		
		_shadow = shadow;
		_shadowcams.RemoveAllObjects();
		
		if(_shadow)
		{
			_shadowSplits  = splits;
			_shadowDistFac = distfac;
			
			TextureParameter parameter;
			parameter.wrapMode = TextureParameter::WrapMode::Clamp;
			parameter.filter = TextureParameter::Filter::Linear;
			parameter.format = TextureParameter::Format::Depth24I;
			parameter.type = TextureParameter::Type::Texture2DArray;
			parameter.depthCompare = true;
			parameter.generateMipMaps = false;
			parameter.mipMaps = 0;
			
			Texture *depthtex = new Texture(parameter);
			depthtex->SetDepth(splits);
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
				
				Camera *tempcam = new Camera(Vector2(resolution), storage, Camera::FlagUpdateAspect | Camera::FlagUpdateStorageFrame | Camera::FlagOrthogonal | Camera::FlagHidden, 1.0f);
				tempcam->SetMaterial(depthMaterial);
				tempcam->SetLODCamera(_lightcam);
				tempcam->SetPriority(kRNShadowCameraPriority);
				tempcam->clipnear = 1.0f;

				_shadowcams.AddObject(tempcam);
				
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
		_shadowcams.RemoveAllObjects();
		
		if(_shadow)
		{
			TextureParameter parameter;
			parameter.wrapMode = TextureParameter::WrapMode::Clamp;
			parameter.filter = TextureParameter::Filter::Linear;
			parameter.format = TextureParameter::Format::Depth24I;
			parameter.type = TextureParameter::Type::TextureCube;
			parameter.depthCompare = false;
			parameter.generateMipMaps = false;
			parameter.mipMaps = 0;
			
			Texture *depthtex = new Texture(parameter);
			depthtex->Autorelease();
			
			Shader   *depthShader = ResourcePool::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyPointShadowDepthShader);
			Material *depthMaterial = new Material(depthShader);
			
			RenderStorage *storage = new RenderStorage(RenderStorage::BufferFormatDepth, 0, 1.0f);
			storage->SetDepthTarget(depthtex, -1);
			
			_shadowcam = new CubemapCamera(Vector2(resolution), storage, Camera::FlagUpdateAspect | Camera::FlagUpdateStorageFrame | Camera::FlagHidden, 1.0f);
			_shadowcam->Retain();
			_shadowcam->Autorelease();
			_shadowcam->SetMaterial(depthMaterial);
			_shadowcam->SetPriority(kRNShadowCameraPriority);
			_shadowcam->clipnear = 0.01f;
			_shadowcam->clipfar = _range;
			_shadowcam->fov = 90.0f;
			_shadowcam->UpdateProjection();
			_shadowcam->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
			storage->Release();
		}
	}
	
	bool Light::CanUpdate(FrameID frame)
	{
		if(_shadow && _lightcam)
		{
			if(_lightcam->GetLastFrame() != frame)
				return false;
			
			if(_shadowcam && _shadowcam->GetLastFrame() != frame)
				return false;
			
			for(int i = 0; i < _shadowSplits; i++)
			{
				Camera *tempcam = _shadowcams.GetObjectAtIndex<Camera>(i);
				
				if(tempcam->GetLastFrame() != frame)
					return false;
			}
			
			return true;
		}
		
		return true;
	}
	
	void Light::Update(float delta)
	{
		SceneNode::Update(delta);
		
		if(_lightType == Type::DirectionalLight)
		{
			if(_shadow && _lightcam)
			{
				float near = _lightcam->clipnear;
				float far;
				
				if(_shadowcam)
					_shadowcam->SetRotation(GetRotation());
				
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
						tempcam->SetRotation(GetRotation());
						
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
				_shadowcam->SetPosition(GetPosition());
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
