//
//  RNEntity.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLightEntity.h"
#include "RNWorld.h"
#include "RNKernel.h"

namespace RN
{
	LightEntity::LightEntity()
		:Entity(Entity::Light)
	{
		
	}
	
	LightEntity::LightEntity(LightEntity *other)
		:Entity(other)
	{
		
	}
	
	LightEntity::~LightEntity()
	{
		
	}
	
	void LightEntity::Update(float delta)
	{
	}
	
	void LightEntity::PostUpdate()
	{
		
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
