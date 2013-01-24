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
		_mesh = 0;
		_material = 0;
		
		World::SharedInstance()->AddEntity(this);
	}
	
	Entity::Entity(Entity *other)
	{
		_mesh = other->_mesh;
		_mesh->Retain();
		
		_material = other->_material;
		_material->Retain();
		
		SetPosition(other->Position());
		SetScale(other->Scale());
		SetRotation(other->Rotation());
		
		World::SharedInstance()->AddEntity(this);
	}
	
	Entity::~Entity()
	{
		World::SharedInstance()->RemoveEntity(this);
		
		_mesh->Release();
		_material->Release();
	}
	
	void Entity::Update(float delta)
	{
	}
	
	void Entity::PostUpdate()
	{
		
	}
	
	void Entity::SetMesh(class Mesh *mesh)
	{
		_mesh->Release();
		_mesh = mesh;
		_mesh->Retain();
	}
	
	void Entity::SetMaterial(class Material *material)
	{
		_material->Release();
		_material = material;
		_material->Retain();
	}
}
