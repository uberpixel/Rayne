//
//  RNLight.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
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
	}
	
	Light::~Light()
	{}

	void Light::Update(float delta)
	{
		Transform::Update(delta);
		_direction = WorldRotation().RotateVector(Vector3(0.0, 0.0, -1.0));
	}
	
	bool Light::IsVisibleInCamera(Camera *camera)
	{
		return camera->InFrustum(Position(), _range);
	}
	
	
	void Light::SetRange(float range)
	{
		_range = range;
	}
	
	void Light::SetColor(Vector3 color)
	{
		_color = color;
	}
	
	void Light::SetAngle(float angle)
	{
		_angle = angle;
	}
}
