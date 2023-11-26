//
//  RNJoltCollisionObject.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltCollisionObject.h"
#include "RNJoltWorld.h"
#include "RNJoltInternals.h"


namespace RN
{
	RNDefineMeta(JoltCollisionObject, SceneNodeAttachment)
		
		JoltCollisionObject::JoltCollisionObject() :
		_collisionFilterGroup(0),
		_collisionFilterMask(0xffffffff),
		_collisionFilterID(0),
		_collisionFilterIgnoreID(0),
		_owner(nullptr)
	{}
		
	JoltCollisionObject::~JoltCollisionObject()
	{
	}
		
		
	void JoltCollisionObject::SetCollisionFilter(uint32 group, uint32 mask)
	{
		_collisionFilterGroup = group;
		_collisionFilterMask = mask;
	}

	void JoltCollisionObject::SetCollisionFilterID(uint32 id, uint32 ignoreid)
	{
		_collisionFilterID = id;
		_collisionFilterIgnoreID = ignoreid;
	}
		
	void JoltCollisionObject::SetContactCallback(std::function<void (JoltCollisionObject *, const JoltContactInfo&, ContactState)> &&callback)
	{
		_contactCallback = std::move(callback);
	}
		
	void JoltCollisionObject::SetPositionOffset(RN::Vector3 offset)
	{
		_positionOffset = offset;
		UpdatePosition();
	}

	void JoltCollisionObject::SetRotationOffset(RN::Quaternion offset)
	{
		_rotationOffset = offset;
		UpdatePosition();
	}
		
		
	void JoltCollisionObject::DidUpdate(SceneNode::ChangeSet changeSet)
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

