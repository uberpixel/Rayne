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

namespace RN
{
	class PhysXWorld;
	class PhysXCollisionObject;

	struct PhysXContactInfo
	{
		SceneNode *node;
		PhysXCollisionObject *collisionObject;
		Vector3 position;
		Vector3 normal;
		float distance;
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
		PXAPI void SetContactCallback(std::function<void(PhysXCollisionObject *, const PhysXContactInfo&, ContactState)> &&callback);
		PXAPI virtual void SetPositionOffset(RN::Vector3 offset);
		PXAPI virtual void SetRotationOffset(RN::Quaternion offset);
		
		uint32 GetCollisionFilterGroup() const { return _collisionFilterGroup; }
		uint32 GetCollisionFilterMask() const { return _collisionFilterMask; }
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		
		Vector3 _positionOffset;
		Quaternion _rotationOffset;

		uint32 _collisionFilterGroup;
		uint32 _collisionFilterMask;
		uint32 _collisionFilterID;
		uint32 _collisionFilterIgnoreID;

		SceneNode *_owner;
			
	private:
		std::function<void(PhysXCollisionObject *, const PhysXContactInfo&, ContactState)> _contactCallback;
			
		RNDeclareMetaAPI(PhysXCollisionObject, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXCOLLISIONOBJECT_H_) */
