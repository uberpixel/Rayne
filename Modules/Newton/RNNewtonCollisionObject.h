//
//  RNNewtonCollisionObject.h
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_NEWTONCOLLISIONOBJECT_H_
#define __RAYNE_NEWTONCOLLISIONOBJECT_H_

#include "RNNewton.h"
#include "RNNewtonShape.h"

namespace RN
{
	class NewtonWorld;

	struct NewtonContactInfo
	{
		SceneNode *node;
		Vector3 position;
		Vector3 normal;
		float distance;
	};
		
	class NewtonCollisionObject : public SceneNodeAttachment
	{
	public:
		friend class NewtonWorld;

		enum ContactState
		{
			Begin,
			Continue,
			End
		};
			
		NDAPI NewtonCollisionObject();
		NDAPI ~NewtonCollisionObject() override;

		NDAPI virtual void UpdatePosition() = 0;
			
		NDAPI virtual void SetCollisionFilter(uint32 group, uint32 mask);
		NDAPI virtual void SetCollisionFilterID(uint32 id, uint32 ignoreid);
		NDAPI void SetContactCallback(std::function<void(NewtonCollisionObject *, const NewtonContactInfo&, ContactState)> &&callback);
		NDAPI virtual void SetPositionOffset(RN::Vector3 offset);
		
		uint32 GetCollisionFilterGroup() const { return _collisionFilterGroup; }
		uint32 GetCollisionFilterMask() const { return _collisionFilterMask; }
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		
		Vector3 _offset;

		uint32 _collisionFilterGroup;
		uint32 _collisionFilterMask;
		uint32 _collisionFilterID;
		uint32 _collisionFilterIgnoreID;

		SceneNode *_owner;
			
	private:
		std::function<void(NewtonCollisionObject *, const NewtonContactInfo&, ContactState)> _contactCallback;
			
		RNDeclareMetaAPI(NewtonCollisionObject, NDAPI)
	};
}

#endif /* defined(__RAYNE_NEWTONCOLLISIONOBJECT_H_) */
