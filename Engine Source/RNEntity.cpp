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
#include "RNSkeleton.h"

namespace RN
{
	RNDeclareMeta(Entity)
	
	Entity::Entity()
	{
		_model = 0;
		_skeleton = 0;
		_type = TypeObject;
	}
	
	Entity::Entity(EntityType type)
	{
		_model = 0;
		_skeleton = 0;
		_type = type;
	}
	
	Entity::~Entity()
	{
		_model->Release();
		_skeleton->Release();
	}
	
	void Entity::Update(float delta)
	{
		Transform::Update(delta);
		
		if(_action)
			_action(this, delta);
	}
	
	bool Entity::IsVisibleInCamera(Camera *camera)
	{
		return true;
	}
	
	void Entity::SetModel(class Model *model)
	{
		_model->Release();
		_model = model->Retain();
	}
	
	void Entity::SetSkeleton(class Skeleton *skeleton)
	{
		_skeleton->Release();
		_skeleton = (class Skeleton *)skeleton->Retain();
	}
	
	void Entity::SetAction(const std::function<void (Entity *, float)>& action)
	{
		_action = action;
	}
}
