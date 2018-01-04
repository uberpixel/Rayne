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

	struct PhysXContactInfo
	{
		SceneNode *node;
		Vector3 position;
		Vector3 normal;
		float distance;
	};
		
	class PhysXCollisionObject : public SceneNodeAttachment
	{
	public:
		friend class PhysXWorld;
		friend class PhysXSimulationCallback;

		enum ContactState
		{
			Begin,
			Continue,
			End
		};
			
		PXAPI PhysXCollisionObject();
		PXAPI ~PhysXCollisionObject() override;

		virtual void UpdatePosition() {}
			
		PXAPI virtual void SetCollisionFilter(uint32 group, uint32 mask);
		PXAPI void SetContactCallback(std::function<void(PhysXCollisionObject *, const PhysXContactInfo&, ContactState)> &&callback);
		PXAPI virtual void SetPositionOffset(RN::Vector3 offset);
		
		uint32 GetCollisionFilterGroup() const { return _collisionFilterGroup; }
		uint32 GetCollisionFilterMask() const { return _collisionFilterMask; }
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		
		Vector3 offset;

		uint32 _collisionFilterGroup;
		uint32 _collisionFilterMask;

		SceneNode *_owner;
			
	private:
		std::function<void(PhysXCollisionObject *, const PhysXContactInfo&, ContactState)> _contactCallback;
			
		RNDeclareMetaAPI(PhysXCollisionObject, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXCOLLISIONOBJECT_H_) */
