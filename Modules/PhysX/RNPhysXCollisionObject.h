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
#include "RNPhysXWorld.h"

namespace RN
{
	class PhysXMaterial;
		
	class PhysXCollisionObject : public SceneNodeAttachment
	{
	public:
		friend class PhysXWorld;
			
		PXAPI PhysXCollisionObject();
		PXAPI ~PhysXCollisionObject() override;
			
		PXAPI void SetCollisionFilter(short int filter);
		PXAPI void SetCollisionFilterMask(short int mask);
//		PXAPI void SetMaterial(BulletMaterial *material);
//		PXAPI void SetContactCallback(std::function<void(BulletCollisionObject *, const BulletContactInfo&)> &&callback);
//		PXAPI void SetSimulationCallback(std::function<void()> &&callback);
		PXAPI virtual void SetPositionOffset(RN::Vector3 offset);
			
		short int GetCollisionFilter() const { return _collisionFilter; }
		short int GetCollisionFilterMask() const { return _collisionFilterMask; }
//		PhysXMaterial *GetMaterial() const { return _material; }
			
//		PXAPI virtual btCollisionObject *GetBulletCollisionObject() const = 0;
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		//void DidAddToParent() override;
		//void WillRemoveFromParent() override;
		
		PhysXWorld *GetOwner() const { return _owner; }
		void ReInsertIntoWorld();
//		virtual void UpdateFromMaterial(PhysXMaterial *material) = 0;
		virtual void InsertIntoWorld(PhysXWorld *world);
		virtual void RemoveFromWorld(PhysXWorld *world);
		Vector3 offset;
			
	private:
//		Connection *_connection;
		PhysXWorld *_owner;
//		PhysXMaterial *_material;
			
//		std::function<void(BulletCollisionObject *, const BulletContactInfo&)> _contactCallback;
//		std::function<void()> _simulationStepCallback;
			
		short int _collisionFilter;
		short int _collisionFilterMask;
			
		RNDeclareMetaAPI(PhysXCollisionObject, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXCOLLISIONOBJECT_H_) */
