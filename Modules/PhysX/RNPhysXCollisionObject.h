//
//  RNPhysXCollisionObject.h
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSXCOLLISIONOBJECT_H_
#define __RAYNE_PHYSXCOLLISIONOBJECT_H_

#include "RNPhysX.h"

namespace physx
{
	class PxRigidActor;
}

namespace RN
{
	class PhysXWorld;
	class PhysXCollisionObject;
	class PhysXShape;

	struct PhysXContactInfo
	{
		SceneNode *node = nullptr;
		PhysXCollisionObject *collisionObject = nullptr;
		Vector3 position;
		Vector3 normal;
		float distance = -1.0f;

		PhysXShape *shapeSelf = nullptr;
		PhysXShape *shapeOther = nullptr;
	};

	class PhysXCollisionObject : public SceneNodeAttachment
	{
	public:
		friend class PhysXWorld;
		friend class PhysXSimulationCallback;
		friend class PhysXKinematicControllerCallback;

		enum ContactState
		{
			Begin,
			Continue,
			End
		};

		PXAPI PhysXCollisionObject();
		PXAPI ~PhysXCollisionObject() override;

		PXAPI virtual void UpdatePosition() = 0;

		PXAPI virtual void SetCollisionFilter(uint32 group, uint32 mask);
		PXAPI virtual void SetCollisionFilterID(uint32 id, uint32 ignoreid);
		PXAPI void SetContactCallback(std::function<void(PhysXCollisionObject *, const PhysXContactInfo &, ContactState)> &&callback);
		PXAPI virtual void SetPositionOffset(RN::Vector3 offset);
		PXAPI virtual void SetRotationOffset(RN::Quaternion offset);

		uint32 GetCollisionFilterGroup() const { return _collisionFilterGroup; }
		uint32 GetCollisionFilterMask() const { return _collisionFilterMask; }

		PXAPI virtual AABB GetPhysXAABB(float inflation = 1.0f) const;
		PXAPI virtual void GetPhysXTransform(Vector3 &position, Quaternion &rotation) const;

		PXAPI virtual physx::PxRigidActor *GetPhysXRigidActor() const = 0;

	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		virtual void ApplyPose() {}
		bool TryMarkPoseUpdateQueued(size_t slot);
		bool ClearPoseUpdateQueued(size_t slot);
		bool IsPoseUpdateQueued() const;

		Vector3 _positionOffset;
		Quaternion _rotationOffset;

		uint32 _collisionFilterGroup;
		uint32 _collisionFilterMask;
		uint32 _collisionFilterID;
		uint32 _collisionFilterIgnoreID;

		SceneNode *_owner;

		static constexpr size_t kInvalidPoseQueueSlot = std::numeric_limits<size_t>::max();
		std::atomic<size_t> _poseUpdateQueueSlot{kInvalidPoseQueueSlot};

	private:
		std::function<void(PhysXCollisionObject *, const PhysXContactInfo &, ContactState)> _contactCallback;

		RNDeclareMetaAPI(PhysXCollisionObject, PXAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_PHYSXCOLLISIONOBJECT_H_) */
