//
//  RNJoltCollisionObject.h
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLTCOLLISIONOBJECT_H_
#define __RAYNE_JOLTCOLLISIONOBJECT_H_

#include "RNJolt.h"

namespace RN
{
	class JoltWorld;
	class JoltCollisionObject;

	struct JoltContactInfo
	{
		SceneNode *node;
		JoltCollisionObject *collisionObject;
		Vector3 position;
		Vector3 normal;
		float distance;
	};
		
	class JoltCollisionObject : public SceneNodeAttachment
	{
	public:
		friend class JoltWorld;
		friend class JoltSimulationCallback;
		friend class JoltKinematicControllerCallback;

		enum ContactState
		{
			Begin,
			Continue,
			End
		};
			
		JTAPI JoltCollisionObject();
		JTAPI ~JoltCollisionObject() override;

		JTAPI virtual void UpdatePosition() = 0;
			
		JTAPI virtual void SetCollisionFilter(uint32 group, uint32 mask);
		JTAPI void SetContactCallback(std::function<void(JoltCollisionObject *, const JoltContactInfo&, ContactState)> &&callback);
		JTAPI virtual void SetPositionOffset(RN::Vector3 offset);
		JTAPI virtual void SetRotationOffset(RN::Quaternion offset);
		
		uint32 GetCollisionFilterGroup() const { return _collisionFilterGroup; }
		uint32 GetCollisionFilterMask() const { return _collisionFilterMask; }
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		
		Vector3 _positionOffset;
		Quaternion _rotationOffset;

		uint32 _collisionFilterGroup;
		uint32 _collisionFilterMask;

		SceneNode *_owner;
			
	private:
		std::function<void(JoltCollisionObject *, const JoltContactInfo&, ContactState)> _contactCallback;
			
		RNDeclareMetaAPI(JoltCollisionObject, JTAPI)
	};
}

#endif /* defined(__RAYNE_JOLTCOLLISIONOBJECT_H_) */
