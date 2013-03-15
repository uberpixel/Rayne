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
	LightEntity::LightEntity(Type lighttype) :
		Entity(Entity::TypeLight), _lightType(lighttype)
	{
		_color = Vector3(1.0f, 1.0f, 1.0f);
		_range = 1.0f;
		_angle = 0.5f;
	}
	
	LightEntity::LightEntity(LightEntity *other) :
		Entity(other)
	{
		_lightType = other->_lightType;
		_color = other->_color;
		_range = other->_range;
		_direction = other->_direction;
		_angle = other->_angle;
	}
	
	LightEntity::~LightEntity()
	{}

	void LightEntity::DidUpdate()
	{
		Entity::DidUpdate();
		_direction = WorldRotation().RotateVector3(Vector3(0.0, 0.0, -1.0));
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
