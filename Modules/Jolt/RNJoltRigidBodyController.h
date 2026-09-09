//
//  RNJoltRigidBodyController.h
//  Rayne-Jolt
//
//  Copyright 2026 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLTRIGIDBODYCONTROLLER_H_
#define __RAYNE_JOLTRIGIDBODYCONTROLLER_H_

#include "RNJolt.h"
#include "RNJoltCollisionObject.h"
#include "RNJoltShape.h"

namespace JPH
{
	class Character;
	class Constraint;
}

namespace RN
{
	class JoltRigidBodyController : public JoltCollisionObject, private JoltConstraintOwner
	{
	public:
		JTAPI JoltRigidBodyController(float radius, float height, float groundTolerance = 0.05f, float mass = 80.0f, float stepOffset = 0.0f);
		JTAPI ~JoltRigidBodyController() override;

		JTAPI void UpdatePosition() override;
		JTAPI uint32 GetJoltBodyID() const;

		JTAPI void Move(const Vector3 &velocity, float delta);
		JTAPI void SetLinearVelocity(const Vector3 &velocity);
		JTAPI void SetMaxLinearVelocity(float max);
		JTAPI void ApplyGravity(const Vector3 &gravity);
		JTAPI bool Resize(float height, bool checkIfBlocked = true);
		JTAPI void SetStepOffset(float stepOffset) { _stepOffset = stepOffset; }
		JTAPI void SetUpDirection(const Vector3 &upDirection);
		JTAPI void SetExternalSupportAnchor(uint32 bodyID, const Vector3 &localPosition, float maxForce);
		JTAPI void ClearExternalSupportAnchor();

		JTAPI void SetCollisionFilter(uint32 group, uint32 mask) override;
		JTAPI Vector3 GetFeetOffset() const;
		JTAPI float GetStepOffset() const { return _stepOffset; }
		JTAPI Vector3 GetUpDirection() const { return _upDirection; }
		JTAPI Vector3 GetGroundVelocity() const { return _groundVelocity; }
		JTAPI Vector3 GetGroundAngularVelocity() const { return _groundAngularVelocity; }
		JTAPI Vector3 GetGroundNormal() const { return _groundNormal; }
		JTAPI Vector3 GetLinearVelocity() const;

		JoltShape *GetShape() const { return _shape; }
		SceneNode *GetObjectBelow() const { return _objectBelow; }
		bool GetIsFalling() const { return _isFalling; }
		uint32 GetContactResponseSupportBodyID() const override { return _externalSupportAnchorValid ? _externalSupportBodyID : 0xffffffff; }

	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;

		JoltShape *_shape;
		JPH::Character *_controller;

		float _radius;
		float _height;
		float _groundTolerance;
		float _mass;
		float _stepOffset;
		Vector3 _upDirection;
		SceneNode *_objectBelow;
		Vector3 _groundVelocity;
		Vector3 _groundAngularVelocity;
		Vector3 _groundNormal;
		bool _isFalling;
		bool _externalSupportAnchorValid;
		bool _externalSupportCollisionFilteringEnabled;
		uint32 _externalSupportBodyID;
		Vector3 _externalSupportLocalPosition;
		float _externalSupportAnchorMaxForce;
		JPH::Constraint *_externalSupportConstraint;

		Vector3 GetGroundAdjustedVelocity(const Vector3 &velocity) const;
		void ApplyStepOffset(const Vector3 &velocity, float delta);
		bool HasBlockingCollisionAt(const JoltPosition &position, const Quaternion &rotation, const Vector3 &movementDirection) const;
		bool HasPenetrationAt(const JoltPosition &position, const Quaternion &rotation, const Vector3 &movementDirection) const;
		bool IsExternalSupportBodyUsable(uint32 bodyID) const;
		bool GetSupportBodyTransform(uint32 bodyID, JoltPosition &position, Quaternion &rotation) const;
		bool GetExternalSupportAnchorState(JoltPosition &position, Vector3 &velocity) const;
		void InvalidateJoltConstraint(JPH::Constraint *constraint) override;
		void UpdateControllerTransform();
		void UpdateGroundState();

		RNDeclareMetaAPI(JoltRigidBodyController, JTAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_JOLTRIGIDBODYCONTROLLER_H_) */
