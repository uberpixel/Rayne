//
//  RNBulletCollisionObject.h
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BULLETCOLLISIONOBJECT_H_
#define __RAYNE_BULLETCOLLISIONOBJECT_H_

#include "RNBullet.h"
#include "RNBulletWorld.h"

class btCollisionObject;

namespace RN
{
	class BulletMaterial;
		
	class BulletCollisionObject : public SceneNodeAttachment
	{
	public:
		friend class BulletWorld;
			
		BTAPI BulletCollisionObject();
		BTAPI ~BulletCollisionObject() override;
			
		BTAPI void SetCollisionFilter(short int filter);
		BTAPI void SetCollisionFilterMask(short int mask);
		BTAPI void SetMaterial(BulletMaterial *material);
		BTAPI void SetContactCallback(std::function<void(BulletCollisionObject *, const BulletContactInfo&)> &&callback);
		BTAPI void SetSimulationCallback(std::function<void()> &&callback);
		BTAPI virtual void SetPositionOffset(RN::Vector3 offset);
			
		short int GetCollisionFilter() const { return _collisionFilter; }
		short int GetCollisionFilterMask() const { return _collisionFilterMask; }
		BulletMaterial *GetMaterial() const { return _material; }
			
		BTAPI virtual btCollisionObject *GetBulletCollisionObject() const = 0;
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		//void DidAddToParent() override;
		//void WillRemoveFromParent() override;
			
		void ReInsertIntoWorld();
		virtual void UpdateFromMaterial(BulletMaterial *material) = 0;
		virtual void InsertIntoWorld(BulletWorld *world);
		virtual void RemoveFromWorld(BulletWorld *world);
		Vector3 offset;
			
	private:
		Connection *_connection;
		BulletWorld *_owner;
		BulletMaterial *_material;
			
		std::function<void(BulletCollisionObject *, const BulletContactInfo&)> _contactCallback;
		std::function<void()> _simulationStepCallback;
			
		short int _collisionFilter;
		short int _collisionFilterMask;
			
		RNDeclareMetaAPI(BulletCollisionObject, BTAPI)
	};
}

#endif /* defined(__RAYNE_BULLETCOLLISIONOBJECT_H_) */
