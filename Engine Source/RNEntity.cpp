//
//  RNEntity.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEntity.h"
#include "RNKernel.h"
#include "RNWorld.h"
#include "RNCamera.h"

namespace RN
{
	Entity::Entity() :
		Transform(Transform::TransformTypeEntity)
	{
		_model = 0;
		_type = TypeObject;
	}
	
	Entity::Entity(EntityType type) :
		Transform(Transform::TransformTypeEntity)
	{
		_model = 0;
		_type = type;
	}
	
	Entity::Entity(Entity *other) :
		Transform(Transform::TransformTypeEntity)
	{
		_model = other->_model->Retain<RN::Model>();
		_type  = other->_type;
		
		SetPosition(other->Position());
		SetScale(other->Scale());
		SetRotation(other->Rotation());
	}
	
	Entity::~Entity()
	{
		_model->Release();
	}
	
	
	
	void Entity::Update(float delta)
	{
		Transform::Update(delta);
	}
	
	void Entity::PostUpdate()
	{
		Transform::PostUpdate();
	}
	
	
	bool Entity::IsVisibleInCamera(Camera *camera)
	{
		return true;
	}
	
	
	void Entity::SetModel(class Model *model)
	{
		_model->Release();
		_model = model->Retain<RN::Model>();
	}
}
