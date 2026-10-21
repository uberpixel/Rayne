//
//  RNJoltCollisionObject.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltCollisionObject.h"
#include "RNJoltInternals.h"
#include "RNJoltWorld.h"


namespace RN
{
	RNDefineMeta(JoltCollisionObject, SceneNodeAttachment)

	JoltCollisionObject::JoltCollisionObject(JoltWorld *world) :
		_collisionFilterGroup(1),
		_collisionFilterMask(0xffffffff),
		_contactResponseMassScaleMask(0),
		_contactResponseInverseMassScale(1.0f),
		_contactResponseInverseInertiaScale(1.0f),
		_contactResponseEstimationEnabled(false),
		_owner(nullptr),
		_world(world ? world : JoltWorld::GetSharedInstance())
	{
		RN_ASSERT(_world, "A Jolt world must exist before creating a collision object.");
	}

	JoltCollisionObject::~JoltCollisionObject()
	{
	}


	void JoltCollisionObject::SetCollisionFilter(uint32 group, uint32 mask)
	{
		_collisionFilterGroup = group;
		_collisionFilterMask = mask;
	}

	void JoltCollisionObject::SetContactCallback(std::function<void(JoltCollisionObject *, const JoltContactInfo &, ContactState)> &&callback)
	{
		_contactCallback = std::move(callback);
	}

	void JoltCollisionObject::SetContactResponseEstimationEnabled(bool enabled)
	{
		_contactResponseEstimationEnabled = enabled;
	}

	void JoltCollisionObject::SetContactResponseMassScale(uint32 collisionMask, float inverseMassScale, float inverseInertiaScale)
	{
		_contactResponseMassScaleMask = collisionMask;
		_contactResponseInverseMassScale = inverseMassScale;
		_contactResponseInverseInertiaScale = inverseInertiaScale;
	}

	float JoltCollisionObject::GetContactResponseInverseMassScaleFor(const JoltCollisionObject *collisionObject) const
	{
		if(!collisionObject || !(_contactResponseMassScaleMask & collisionObject->GetCollisionFilterGroup()))
		{
			return 1.0f;
		}

		return _contactResponseInverseMassScale;
	}

	float JoltCollisionObject::GetContactResponseInverseInertiaScaleFor(const JoltCollisionObject *collisionObject) const
	{
		if(!collisionObject || !(_contactResponseMassScaleMask & collisionObject->GetCollisionFilterGroup()))
		{
			return 1.0f;
		}

		return _contactResponseInverseInertiaScale;
	}

	void JoltCollisionObject::NotifyContact(const JoltContactInfo &info, ContactState state)
	{
		if(_contactCallback)
		{
			_contactCallback(this, info, state);
		}
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
} // namespace RN
