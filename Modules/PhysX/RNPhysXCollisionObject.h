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
			
		PXAPI PhysXCollisionObject();
		PXAPI ~PhysXCollisionObject() override;

		virtual void UpdatePosition() {}
			
		PXAPI virtual void SetCollisionFilter(uint32 group, uint32 mask);
		PXAPI void SetContactCallback(std::function<void(PhysXCollisionObject *, const PhysXContactInfo&)> &&callback);
//		PXAPI void SetSimulationCallback(std::function<void()> &&callback);
		PXAPI virtual void SetPositionOffset(RN::Vector3 offset);
		
		uint32 GetCollisionFilterGroup() const { return _collisionFilterGroup; }
		uint32 GetCollisionFilterMask() const { return _collisionFilterMask; }
			
//		PXAPI virtual btCollisionObject *GetBulletCollisionObject() const = 0;
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		//void DidAddToParent() override;
		//void WillRemoveFromParent() override;
		
		PhysXWorld *GetOwner() const { return _owner; }
		void ReInsertIntoWorld();
		virtual void InsertIntoWorld(PhysXWorld *world);
		virtual void RemoveFromWorld(PhysXWorld *world);
		Vector3 offset;

		uint32 _collisionFilterGroup;
		uint32 _collisionFilterMask;
			
	private:
		PhysXWorld *_owner;
			
		std::function<void(PhysXCollisionObject *, const PhysXContactInfo&)> _contactCallback;
//		std::function<void()> _simulationStepCallback;
			
		RNDeclareMetaAPI(PhysXCollisionObject, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXCOLLISIONOBJECT_H_) */
