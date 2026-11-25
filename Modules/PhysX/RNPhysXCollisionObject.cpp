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
		_owner(nullptr),
		_poseUpdateQueueSlot(kInvalidPoseQueueSlot)
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

	void PhysXCollisionObject::SetContactCallback(std::function<void(PhysXCollisionObject *, const PhysXContactInfo &, ContactState)> &&callback)
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

	bool PhysXCollisionObject::TryMarkPoseUpdateQueued(size_t slot)
	{
		size_t expected = kInvalidPoseQueueSlot;
		return _poseUpdateQueueSlot.compare_exchange_strong(expected, slot, std::memory_order_acq_rel);
	}

	bool PhysXCollisionObject::ClearPoseUpdateQueued(size_t slot)
	{
		size_t expected = slot;
		return _poseUpdateQueueSlot.compare_exchange_strong(expected, kInvalidPoseQueueSlot, std::memory_order_acq_rel);
	}

	bool PhysXCollisionObject::IsPoseUpdateQueued() const
	{
		return _poseUpdateQueueSlot.load(std::memory_order_acquire) != kInvalidPoseQueueSlot;
	}


	void PhysXCollisionObject::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			// If detached, drop any queued pose and balance the enqueue retain.
			if(!GetParent())
			{
				size_t slot = _poseUpdateQueueSlot.load(std::memory_order_acquire);
				if(slot != kInvalidPoseQueueSlot)
				{
					if(PhysXWorld::GetSharedInstance())
					{
						PhysXWorld::GetSharedInstance()->ClearPoseQueueSlot(slot);
					}

					if(ClearPoseUpdateQueued(slot))
					{
						Release();
					}
				}
			}
		}
	}

	AABB PhysXCollisionObject::GetPhysXAABB(RN_UNUSED float inflation) const
	{
		return AABB();
	}

	void PhysXCollisionObject::GetPhysXTransform(Vector3 &position, Quaternion &rotation) const
	{
		position = Vector3();
		rotation = Quaternion();
	}

	void PhysXCollisionObject::ResetFiltering()
	{
		if(physx::PxRigidActor *actor = GetPhysXRigidActor())
		{
			PhysXWorld *world = PhysXWorld::GetSharedInstance();
			world->Lock();
			physx::PxScene *scene = world->GetPhysXScene();
			scene->resetFiltering(*actor);
			world->Unlock();
		}
	}
} // namespace RN
