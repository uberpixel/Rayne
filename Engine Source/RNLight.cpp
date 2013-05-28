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

namespace RN
{
	RNDeclareMeta(Light)
	
	Light::Light(Type lighttype) :
		_lightType(lighttype)
	{
		_color = RN::Color(1.0f, 1.0f, 1.0f);
		_range = 1.0f;
		_angle = 0.5f;
		_intensity = 10.0f;

		_shadow = false;
		_shadowcam = 0;
		_lightcam = 0;

		ReCalculateColor();
	}
	
	Light::~Light()
	{}
	
	bool Light::IsVisibleInCamera(Camera *camera)
	{
		if(_lightType == TypeDirectionalLight)
			return true;
		
		return SceneNode::IsVisibleInCamera(camera);
	}
	
	void Light::Render(Renderer *renderer, Camera *camera)
	{
		renderer->RenderLight(this);
	}
	
	void Light::SetRange(float range)
	{
		_range = range;
		
		SetBoundingSphere(Sphere(Vector3(), range));
		SetBoundingBox(AABB(Vector3(), range));
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
	
	void Light::ActivateSunShadows(bool shadow, float resolution, int splits, float distfac, float biasfac, float biasunits)
	{
		if(_lightType != TypeDirectionalLight)
			return;
		
		if(_shadow == shadow)
			return;
		
		_shadow = shadow;
		if(_shadow)
		{
			_shadowSplits = splits;
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
			
			RenderStorage *storage;
			
/*			try
			{
				storage = new RenderStorage(RenderStorage::BufferFormatDepth);
				storage->SetDepthTarget(depthtex);
				storage->SetFrame(Rect(0.0f, 0.0f, 256.0f, 256.0f));
				
				storage->Bind();
				storage->UpdateBuffer();
				storage->Unbind();
				
				Shader *depthShader = Shader::WithFile("shader/rn_ShadowDepth");
				Material *depthMaterial = new Material(depthShader);
				depthMaterial->polygonOffset = true;
				depthMaterial->polygonOffsetFactor = biasfac;
				depthMaterial->polygonOffsetUnits = biasunits;
				
				_shadowcam = new Camera(Vector2(resolution), storage, Camera::FlagUpdateAspect | Camera::FlagUpdateStorageFrame | Camera::FlagOrthogonal | Camera::FlagHidden);
				
				_shadowcam->SetMaterial(depthMaterial);
				_shadowcam->SetUseInstancing(false);
				_shadowcam->SetLODCamera(_lightcam);
				_shadowcam->clipfar = 1000.0f;
				_shadowcam->clipnear = 1.0f;
			}
			catch(ErrorException e)*/
			{
//				storage->Unbind();
//				storage->Release();
				
				Shader *depthShader = Shader::WithFile("shader/rn_ShadowDepthSingle");
				Material *depthMaterial = new Material(depthShader);
				depthMaterial->polygonOffset = true;
				depthMaterial->polygonOffsetFactor = biasfac;
				depthMaterial->polygonOffsetUnits = biasunits;
				
				for(int i = 0; i < _shadowSplits; i++)
				{
					storage = new RenderStorage(RenderStorage::BufferFormatDepth, 0, 1.0f);
					storage->SetDepthTarget(depthtex, i);
					
					Camera *tempcam = new Camera(Vector2(resolution), storage, Camera::FlagUpdateAspect | Camera::FlagUpdateStorageFrame | Camera::FlagOrthogonal | Camera::FlagHidden, 1.0f);
					tempcam->SetMaterial(depthMaterial);
					tempcam->SetUseInstancing(true);
					tempcam->SetLODCamera(_lightcam);
					tempcam->SetPriority(kRNShadowCameraPriority);
					tempcam->clipnear = 1.0f;
//					tempcam->clipfar = 10000.0f;
	
					_shadowcams.AddObject(tempcam);
				}
			}
		}
	}
	
	void Light::Update(float delta)
	{
		SceneNode::Update(delta);
		
		if(_shadow)
		{
			float near = _lightcam->clipnear;
			float far;
			
			if(_shadowcam != 0)
				_shadowcam->SetRotation(Rotation());
			
			_shadowmats.clear();
			
			for(int i = 0; i < _shadowSplits; i++)
			{
				float linear = _lightcam->clipnear+(_lightcam->clipfar-_lightcam->clipnear)*(i+1.0f)/float(_shadowSplits);
				float log = _lightcam->clipnear*powf(_lightcam->clipfar/_lightcam->clipnear, (i+1.0f)/float(_shadowSplits));
				far = linear*_shadowDistFac+log*(1.0f-_shadowDistFac);
				
				if(_shadowcam != 0)
				{
					_shadowmats.push_back(std::move(_shadowcam->MakeShadowSplit(_lightcam, this, near, far)));
				}
				else
				{
					Camera *tempcam = _shadowcams.ObjectAtIndex<Camera>(i);
					tempcam->SetRotation(Rotation());
					
					_shadowmats.push_back(std::move(tempcam->MakeShadowSplit(_lightcam, this, near, far)));
				}
				
				near = far;
			}
			
			if(_shadowcam != 0)
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
