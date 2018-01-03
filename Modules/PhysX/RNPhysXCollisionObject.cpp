//
//  RNPhysXCollisionObject.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXCollisionObject.h"
#include "RNPhysXWorld.h"

#include "PxPhysicsAPI.h"

namespace RN
{
	RNDefineMeta(PhysXCollisionObject, SceneNodeAttachment)
		
		PhysXCollisionObject::PhysXCollisionObject() :
		_collisionFilterGroup(0),
		_collisionFilterMask(0xffffffff),
		_owner(nullptr)
	{}
		
	PhysXCollisionObject::~PhysXCollisionObject()
	{
	}
		
		
	void PhysXCollisionObject::SetCollisionFilter(uint32 group, uint32 mask)
	{
		_collisionFilterGroup = group;
		_collisionFilterMask = mask;
	}
		
	void PhysXCollisionObject::SetContactCallback(std::function<void (PhysXCollisionObject *, const PhysXContactInfo&, ContactState)> &&callback)
	{
		_contactCallback = std::move(callback);
	}

/*	void PhysXCollisionObject::SetSimulationCallback(std::function<void()> &&callback)
	{
		_simulationStepCallback = std::move(callback);
	}*/
		
	void PhysXCollisionObject::SetPositionOffset(RN::Vector3 offset)
	{
		this->offset = offset;
	}
		
	void PhysXCollisionObject::ReInsertIntoWorld()
	{
/*		if(_owner)
		{
			auto world = _owner;
				
			world->Lock();
//			world->RemoveCollisionObject(this);
//			world->InsertCollisionObject(this);
			world->Unlock();
		}*/
	}
		
	void PhysXCollisionObject::InsertIntoWorld(PhysXWorld *world)
	{
		_owner = world;
	}
	void PhysXCollisionObject::RemoveFromWorld(PhysXWorld *world)
	{
		_owner = nullptr;
	}
		
		
	void PhysXCollisionObject::DidUpdate(SceneNode::ChangeSet changeSet)
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
		
/*	void PhysXCollisionObject::DidAddToParent()
	{
		if(!_owner)
			BulletWorld::GetSharedInstance()->InsertCollisionObject(this);
	}
		
	void PhysXCollisionObject::WillRemoveFromParent()
	{
		if(_owner)
			_owner->RemoveCollisionObject(this);
	}*/
}
