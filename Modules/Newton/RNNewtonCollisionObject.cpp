//
//  RNNewtonCollisionObject.cpp
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNNewtonCollisionObject.h"
#include "RNNewtonWorld.h"

namespace RN
{
	RNDefineMeta(NewtonCollisionObject, SceneNodeAttachment)
		
		NewtonCollisionObject::NewtonCollisionObject() :
		_collisionFilterGroup(0),
		_collisionFilterMask(0xffffffff),
		_collisionFilterID(0),
		_collisionFilterIgnoreID(0),
		_owner(nullptr)
	{}
		
	NewtonCollisionObject::~NewtonCollisionObject()
	{
	}
		
		
	void NewtonCollisionObject::SetCollisionFilter(uint32 group, uint32 mask)
	{
		_collisionFilterGroup = group;
		_collisionFilterMask = mask;
	}

	void NewtonCollisionObject::SetCollisionFilterID(uint32 id, uint32 ignoreid)
	{
		_collisionFilterID = id;
		_collisionFilterIgnoreID = ignoreid;
	}
		
	void NewtonCollisionObject::SetContactCallback(std::function<void (NewtonCollisionObject *, const NewtonContactInfo&, ContactState)> &&callback)
	{
		_contactCallback = std::move(callback);
	}
		
	void NewtonCollisionObject::SetPositionOffset(RN::Vector3 offset)
	{
		_offset = offset;
		UpdatePosition();
	}
		
		
	void NewtonCollisionObject::DidUpdate(SceneNode::ChangeSet changeSet)
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
