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
		_collisionFilterID(0),
		_collisionFilterIgnoreID(0),
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

	void PhysXCollisionObject::SetCollisionFilterID(uint32 id, uint32 ignoreid)
	{
		_collisionFilterID = id;
		_collisionFilterIgnoreID = ignoreid;
	}
		
	void PhysXCollisionObject::SetContactCallback(std::function<void (PhysXCollisionObject *, const PhysXContactInfo&, ContactState)> &&callback)
	{
		_contactCallback = std::move(callback);
	}
		
	void PhysXCollisionObject::SetPositionOffset(RN::Vector3 offset)
	{
		_positionOffset = offset;
		UpdatePosition();
	}

	void PhysXCollisionObject::SetRotationOffset(RN::Quaternion offset)
	{
		_rotationOffset = offset;
		UpdatePosition();
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
}
