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
		_shadowcam = NULL;
	}
	
	Light::~Light()
	{}
	
	const Vector3& Light::Direction()
	{
		_direction = WorldRotation().RotateVector(Vector3(0.0, 0.0, 1.0));
		return _direction;
	}
	
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
			RenderStorage *storage = new RenderStorage(RenderStorage::BufferFormatColor | RenderStorage::BufferFormatDepth | RenderStorage::BufferFormatStencil);
			Texture *depthtex = new Texture(Texture::FormatDepthStencil);
			
			storage->SetDepthTarget(depthtex);
			storage->AddRenderTarget(Texture::FormatRGBA8888);
			
			Shader *depthShader = Shader::WithFile("shader/rn_LightDepth");
			Material *depthMaterial = new Material(depthShader);
			
			_shadowcam = new Camera(Vector2(1024, 1024), storage, Camera::FlagUpdateAspect | Camera::FlagUpdateStorageFrame | Camera::FlagOrthogonal | Camera::FlagHidden);
			_shadowcam->SetMaterial(depthMaterial);
			_shadowcam->SetPosition(Position());
			_shadowcam->SetRotation(Rotation());
			_shadowcam->UpdateProjection();
			_shadowcam->Update(0.1f);
			_shadowcam->PostUpdate();
//			AttachChild(_shadowcam);
		}
	}
}
