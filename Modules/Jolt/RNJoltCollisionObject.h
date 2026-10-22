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

	struct JoltEstimatedContactPointImpulse
	{
		JoltPosition position;
		Vector3 impulse; // Estimated impulse applied to the collision object receiving the callback.
		float normalImpulse = 0.0f; // Magnitude along the contact manifold normal before applying the per-body sign.
	};

	struct JoltContactInfo
	{
		SceneNode *node = nullptr;
		JoltCollisionObject *collisionObject = nullptr;
		JoltPosition position;
		Vector3 normal;
		Vector3 linearVelocity;
		Vector3 otherLinearVelocity;
		Vector3 relativeContactVelocity;
		float distance = 0.0f; // Ray/sweep distance, or penetration depth for overlap queries.
		float contactPatchRadius = 0.0f;
		uint64 subShapeUserData = 0;
		uint32 compoundChildIndex = 0xffffffffU;
		uint32 compoundChildUserData = 0;
		uint64 ownSubShapeUserData = 0;
		uint32 ownCompoundChildIndex = 0xffffffffU;
		uint32 ownCompoundChildUserData = 0;
		std::vector<JoltEstimatedContactPointImpulse> estimatedContactPointImpulses; // Populated for Begin contacts when estimation is enabled.
		float estimatedContactImpulseTimeStep = 0.0f; // Physics step duration used to estimate these impulses.
	};

	class JoltCollisionObject : public SceneNodeAttachment
	{
	public:
		friend class JoltWorld;
		friend class JoltSimulationCallback;
		friend class JoltCharacterContactListener;

		enum ContactState
		{
			Begin,
			Continue,
			End
		};

		JTAPI JoltCollisionObject(JoltWorld *world = nullptr);
		JTAPI ~JoltCollisionObject() override;

		// The owning world must outlive this attachment. A null constructor world captures the legacy default.
		JoltWorld *GetJoltWorld() const { return _world; }

		JTAPI virtual void UpdatePosition() = 0;

		JTAPI virtual void SetCollisionFilter(uint32 group, uint32 mask);
		JTAPI void SetContactCallback(std::function<void(JoltCollisionObject *, const JoltContactInfo &, ContactState)> &&callback);
		JTAPI std::function<void(JoltCollisionObject *, const JoltContactInfo &, ContactState)> GetContactCallback() const { return _contactCallback; }
		JTAPI void SetContactResponseEstimationEnabled(bool enabled);
		bool GetContactResponseEstimationEnabled() const { return _contactResponseEstimationEnabled; }
		JTAPI void SetContactResponseMassScale(uint32 collisionMask, float inverseMassScale, float inverseInertiaScale);
		JTAPI float GetContactResponseInverseMassScaleFor(const JoltCollisionObject *collisionObject) const;
		JTAPI float GetContactResponseInverseInertiaScaleFor(const JoltCollisionObject *collisionObject) const;
		JTAPI virtual uint32 GetContactResponseSupportBodyID() const { return 0xffffffff; }
		JTAPI virtual void SetPositionOffset(RN::Vector3 offset);
		JTAPI virtual void SetRotationOffset(RN::Quaternion offset);
		JTAPI void NotifyContact(const JoltContactInfo &info, ContactState state);

		Vector3 GetPositionOffset() const { return _positionOffset; }

		uint32 GetCollisionFilterGroup() const { return _collisionFilterGroup; }
		uint32 GetCollisionFilterMask() const { return _collisionFilterMask; }

	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		void SetJoltWorld(JoltWorld *world);

		Vector3 _positionOffset;
		Quaternion _rotationOffset;

		uint32 _collisionFilterGroup;
		uint32 _collisionFilterMask;
		uint32 _contactResponseMassScaleMask;
		float _contactResponseInverseMassScale;
		float _contactResponseInverseInertiaScale;
		bool _contactResponseEstimationEnabled;

		SceneNode *_owner;
		JoltWorld *_world;

	private:
		std::function<void(JoltCollisionObject *, const JoltContactInfo &, ContactState)> _contactCallback;

		RNDeclareMetaAPI(JoltCollisionObject, JTAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_JOLTCOLLISIONOBJECT_H_) */
