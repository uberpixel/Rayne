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
	LightEntity::LightEntity() :
		Entity(Entity::TypeLight)
	{
		_color = Vector3(1.0f, 1.0f, 1.0f);
		_range = 1.0f;
	}
	
	LightEntity::LightEntity(LightEntity *other) :
		Entity(other)
	{
		_color = other->_color;
		_range = other->_range;
	}
	
	LightEntity::~LightEntity()
	{}
	
	
	
	void LightEntity::Update(float delta)
	{
	}
	
	void LightEntity::PostUpdate()
	{
		
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
}
