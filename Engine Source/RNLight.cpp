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
		_lightType(lighttype)
	{
		_color = Color(1.0f, 1.0f, 1.0f);
		_range = 1.0f;
		_angle = 0.5f;
		_intensity = 10.0f;

		_shadow = false;
		_shadowcam = nullptr;
		_lightcam  = nullptr;
		
		collisionGroup = 25;

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
	
	void Light::SetColor(const class Color& color)
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
	
	void Light::SetShadowCamera(Camera *shadowCamera)
	{
		if(_shadowcam)
			_shadowcam->Release();
		
		_shadowcam = shadowCamera ? shadowCamera->Retain() : nullptr;
	}
	
	void Light::SetLightCamera(Camera *lightCamera)
	{
		if(_lightcam)
			_lightcam->Release();
		
		_lightcam = lightCamera ? lightCamera->Retain() : nullptr;
	}
	
	void Light::ActivateSunShadows(bool shadow, float resolution, int splits, float distfac, float biasfac, float biasunits)
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
			parameter.format = TextureParameter::Format::Depth;
			parameter.type = TextureParameter::Type::Texture2DArray;
			parameter.depthCompare = true;
			parameter.generateMipMaps = false;
			parameter.mipMaps = 0;
			
			Texture *depthtex = new Texture(parameter);
			depthtex->SetDepth(splits);
			depthtex->Autorelease();
			
			Shader   *depthShader = ResourcePool::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyShadowDepthShader);
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
//				tempcam->clipfar = 10000.0f;

				_shadowcams.AddObject(tempcam);
				
				tempcam->Release();
				storage->Release();
			}
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

	void Light::ReCalculateColor()
	{
		_resultColor = Vector3(_color.r, _color.g, _color.b);
		_resultColor *= _intensity;
	}
}
