//
//  RNODECollisionObject.cpp
//  Rayne-ODE
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNODECollisionObject.h"

namespace RN
{
	RNDefineMeta(ODECollisionObject, SceneNodeAttachment)
		
	ODECollisionObject::ODECollisionObject() :
//		_collisionFilter(btBroadphaseProxy::DefaultFilter),
//		_collisionFilterMask(btBroadphaseProxy::AllFilter),
		_owner(nullptr),
//		_material(nullptr),
		_contactCallback(nullptr),
		_simulationStepCallback(nullptr)
	{}
		
	ODECollisionObject::~ODECollisionObject()
	{
/*		if(_material)
		{
			_connection->Disconnect();
			_material->Release();
		}*/
	}
		
		
	void ODECollisionObject::SetCollisionFilter(short int filter)
	{
		_collisionFilter = filter;
		ReInsertIntoWorld();
	}
	void ODECollisionObject::SetCollisionFilterMask(short int mask)
	{
		_collisionFilterMask = mask;
		ReInsertIntoWorld();
	}
/*	void ODECollisionObject::SetMaterial(BulletMaterial *tmaterial)
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
	}*/
		
	void ODECollisionObject::SetContactCallback(std::function<void (ODECollisionObject *, const ODEContactInfo&)> &&callback)
	{
		_contactCallback = std::move(callback);
	}

	void ODECollisionObject::SetSimulationCallback(std::function<void()> &&callback)
	{
		_simulationStepCallback = std::move(callback);
	}
		
	void ODECollisionObject::SetPositionOffset(RN::Vector3 offset)
	{
		this->offset = offset;
	}
		
	void ODECollisionObject::ReInsertIntoWorld()
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
		
	void ODECollisionObject::InsertIntoWorld(ODEWorld *world)
	{
		_owner = world;
	}
	void ODECollisionObject::RemoveFromWorld(ODEWorld *world)
	{
		_owner = nullptr;
	}
		
		
	void ODECollisionObject::DidUpdate(SceneNode::ChangeSet changeSet)
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
