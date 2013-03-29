//
//  RNEntity.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLightEntity.h"
#include "RNWorld.h"
#include "RNCamera.h"

namespace RN
{
	RNDeclareMeta(LightEntity)
	
	LightEntity::LightEntity(Type lighttype) :
		Entity(Entity::TypeLight),
		_lightType(lighttype)
	{
		_color = Vector3(1.0f, 1.0f, 1.0f);
		_range = 1.0f;
		_angle = 0.5f;
	}
	
	LightEntity::~LightEntity()
	{}

	void LightEntity::Update(float delta)
	{
		Entity::Update(delta);
		_direction = WorldRotation().RotateVector(Vector3(0.0, 0.0, -1.0));
	}
	
	bool LightEntity::IsVisibleInCamera(Camera *camera)
	{
		return camera->InFrustum(Position(), _range);
	}
	
	
	void LightEntity::SetRange(float range)
	{
		_range = range;
	}
	
	void LightEntity::SetColor(Vector3 color)
	{
		_color = color;
	}
	
	void LightEntity::SetAngle(float angle)
	{
		_angle = angle;
	}
}
