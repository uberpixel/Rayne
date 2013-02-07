//
//  RNEntity.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEntity.h"
#include "RNWorld.h"
#include "RNKernel.h"

namespace RN
{
	Entity::Entity()
	{
		_model = 0;
		_type = Object;
		
		World::SharedInstance()->AddEntity(this);
	}
	
	Entity::Entity(Type type)
	{
		_model = 0;
		_type = type;
		
		World::SharedInstance()->AddEntity(this);
	}
	
	Entity::Entity(Entity *other)
	{
		_model = other->_model->Retain<RN::Model>();
		_type = other->GetType();
		
		SetPosition(other->Position());
		SetScale(other->Scale());
		SetRotation(other->Rotation());
		
		World::SharedInstance()->AddEntity(this);
	}
	
	Entity::~Entity()
	{
		World::SharedInstance()->RemoveEntity(this);
		
		_model->Release();
	}
	
	void Entity::Update(float delta)
	{
	}
	
	void Entity::PostUpdate()
	{
		
	}
	
	void Entity::SetModel(class Model *model)
	{
		_model->Release();
		_model = model->Retain<RN::Model>();
	}
}
