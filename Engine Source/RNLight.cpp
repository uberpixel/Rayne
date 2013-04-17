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
		_intensity = 1.0;
		
		ReCalculateColor();
	}
	
	Light::~Light()
	{}
	
	const Vector3& Light::Direction()
	{
		_direction = WorldRotation().RotateVector(Vector3(0.0, 0.0, -1.0));
		return _direction;
	}
	
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
	
	void Light::ReCalculateColor()
	{
		_resultColor = Vector3(_color.r, _color.g, _color.b);
		_resultColor *= _intensity;
	}
}
