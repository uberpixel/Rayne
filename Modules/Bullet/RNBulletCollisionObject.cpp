//
//  RNBulletCollisionObject.cpp
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBulletCollisionObject.h"
#include "RNBulletMaterial.h"

#include "btBulletDynamicsCommon.h"

namespace RN
{
	RNDefineMeta(BulletCollisionObject, SceneNodeAttachment)
		
		BulletCollisionObject::BulletCollisionObject() :
		_collisionFilter(btBroadphaseProxy::DefaultFilter),
		_collisionFilterMask(btBroadphaseProxy::AllFilter),
		_owner(nullptr),
		_material(nullptr)
	{}
		
	BulletCollisionObject::~BulletCollisionObject()
	{
		if(_material)
		{
			_connection->Disconnect();
			_material->Release();
		}
	}
		
		
	void BulletCollisionObject::SetCollisionFilter(short int filter)
	{
		_collisionFilter = filter;
		ReInsertIntoWorld();
	}
	void BulletCollisionObject::SetCollisionFilterMask(short int mask)
	{
		_collisionFilterMask = mask;
		ReInsertIntoWorld();
	}
	void BulletCollisionObject::SetMaterial(BulletMaterial *tmaterial)
	{
		if(_material)
		{
			_connection->Disconnect();
			_material->Release();
		}
			
		if((_material = SafeRetain(tmaterial)))
		{
			_connection = _material->signal.Connect(std::bind(&BulletCollisionObject::UpdateFromMaterial, this, std::placeholders::_1));
			UpdateFromMaterial(_material);
		}
	}
		
	void BulletCollisionObject::SetContactCallback(std::function<void (BulletCollisionObject *, const BulletContactInfo&)> &&callback)
	{
		_callback = std::move(callback);
	}
		
	void BulletCollisionObject::SetPositionOffset(RN::Vector3 offset)
	{
		this->offset = offset;
	}
		
	void BulletCollisionObject::ReInsertIntoWorld()
	{
		if(_owner)
		{
			auto world = _owner;
				
			world->Lock();
			world->RemoveCollisionObject(this);
			world->InsertCollisionObject(this);
			world->Unlock();
		}
	}
		
	void BulletCollisionObject::InsertIntoWorld(BulletWorld *world)
	{
		_owner = world;
	}
	void BulletCollisionObject::RemoveFromWorld(BulletWorld *world)
	{
		_owner = nullptr;
	}
		
		
	void BulletCollisionObject::DidUpdate(SceneNode::ChangeSet changeSet)
	{
/*		if(changeSet & SceneNode::ChangeSet::World)
		{
			World *world = GetParent()->GetWorld();
				
			if(!world && _owner)
			{
				_owner->RemoveCollisionObject(this);
				return;
			}
				
			if(world && !_owner)
			{
				BulletWorld::GetSharedInstance()->InsertCollisionObject(this);
				return;
			}
		}*/
	}
		
/*	void BulletCollisionObject::DidAddToParent()
	{
		if(!_owner)
			BulletWorld::GetSharedInstance()->InsertCollisionObject(this);
	}
		
	void BulletCollisionObject::WillRemoveFromParent()
	{
		if(_owner)
			_owner->RemoveCollisionObject(this);
	}*/
}
