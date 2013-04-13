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
		_color = Vector3(1.0f, 1.0f, 1.0f);
		_range = 1.0f;
		_angle = 0.5f;
		_shadow = false;
	}
	
	Light::~Light()
	{}
	
	bool Light::IsVisibleInCamera(Camera *camera)
	{
		if(TypeDirectionalLight)
			return true;
		
		return camera->InFrustum(Position(), _range);
	}
	
	
	void Light::SetRange(float range)
	{
		_range = range;
	}
	
	void Light::SetColor(const Vector3& color)
	{
		_color = color;
	}
	
	void Light::SetAngle(float angle)
	{
		_angle = angle;
	}
	
	void Light::SetShadow(bool shadow)
	{
		if(_shadow == shadow)
			return;
		
		_shadow = shadow;
		if(_shadow)
		{
			Shader *depthShader = Shader::WithFile("shader/rn_LightDepth");
			Material *depthMaterial = new Material(depthShader);
			
			Texture *depthtex = new Texture(Texture::FormatDepth, Texture::WrapModeRepeat, Texture::FilterLinear, false, Texture::Type2DArray);
			depthtex->SetDepth(4);
			
			for(int i = 0; i < 4; i++)
			{
				RenderStorage *storage = new RenderStorage(RenderStorage::BufferFormatDepth);
				storage->SetDepthTarget(depthtex, i);
				Camera *tempcam = new Camera(Vector2(512), storage, Camera::FlagUpdateAspect | Camera::FlagUpdateStorageFrame | Camera::FlagOrthogonal | Camera::FlagHidden);
				tempcam->SetMaterial(depthMaterial);
//				AttachChild(tempcam);
				tempcam->SetRotation(Rotation());
				tempcam->SetLODCamera(_lightcam);
				tempcam->override = RN::Camera::OverrideAll & ~(RN::Camera::OverrideDiscard | RN::Camera::OverrideDiscardThreshold | RN::Camera::OverrideTextures);
				
				_shadowcams.AddObject(tempcam);
			}
		}
	}
	
	void Light::Update(float delta)
	{
		Transform::Update(delta);
		if(_shadow)
		{
			float near = _lightcam->clipnear;
			float far;
			for(int i = 0; i < 4; i++)
			{
				_shadowcams.ObjectAtIndex(i)->SetRotation(Rotation());
				float linear = _lightcam->clipnear+(_lightcam->clipfar-_lightcam->clipnear)*(i+1.0f)/4.0f;
				float log = _lightcam->clipnear*powf(_lightcam->clipfar/_lightcam->clipnear, (i+1.0f)/4.0f);
				far = linear*0.04f+log*0.96f;
				_shadowcams.ObjectAtIndex(i)->MakeShadowSplit(_lightcam, this, near, far);
				near = far;
			}
		}
	}
}
