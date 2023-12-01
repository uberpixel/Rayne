//
//  RNJoltDynamicBody.h
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLTDYNAMICBODY_H_
#define __RAYNE_JOLTDYNAMICBODY_H_

#include "RNJoltCollisionObject.h"

namespace JPH
{
	class BodyID;
}

namespace RN
{
	class JoltShape;
	class JoltDynamicBody : public JoltCollisionObject
	{
	public:
		enum LockAxis
		{
			LockAxisLinearX = (1 << 0),
			LockAxisLinearY = (1 << 1),
			LockAxisLinearZ = (1 << 2),
			LockAxisAngularX = (1 << 3),
			LockAxisAngularY = (1 << 4),
			LockAxisAngularZ = (1 << 5)
		};
		
		JTAPI JoltDynamicBody(JoltShape *shape, float mass);
		JTAPI ~JoltDynamicBody() override;
			
		JTAPI static JoltDynamicBody *WithShape(JoltShape *shape, float mass);

		JTAPI void UpdatePosition() override;

		JTAPI void SetCollisionFilter(uint32 group, uint32 mask) override;
			
		JTAPI void SetMass(float mass);
		JTAPI void SetLinearVelocity(const Vector3 &velocity);
		JTAPI void SetAngularVelocity(const Vector3 &velocity);
		JTAPI void SetDamping(float linear, float angular);
		JTAPI void SetMaxAngularVelocity(float max);
		JTAPI void SetMaxDepenetrationVelocity(float max);
		JTAPI void SetEnableCCD(bool enable);
		JTAPI void SetEnableGravity(bool enable);
		JTAPI void SetEnableKinematic(bool enable);
		JTAPI void LockMovement(uint32 lockFlags);
		JTAPI void SetSolverIterationCount(uint32 positionIterations, uint32 velocityIterations);

		JTAPI void SetKinematicTarget(const Vector3 &position, const Quaternion &rotation);
		//JTAPI void AccelerateToTarget(const Vector3 &position, const Quaternion &rotation, float delta);

		JTAPI void AddForce(const Vector3 &force);
		JTAPI void AddForce(const Vector3 &force, const Vector3 &origin);
//		JTAPI void ClearForces();
			
		JTAPI void AddTorque(const Vector3 &torque);
		JTAPI void AddTorqueImpulse(const Vector3 &torque);
		JTAPI void AddImpulse(const Vector3 &impulse);
		JTAPI void AddImpulse(const Vector3 &impulse, const Vector3 &origin);
			
		JTAPI float GetMass() const;
		
		JTAPI Vector3 GetLinearVelocity() const;
		JTAPI Vector3 GetAngularVelocity() const;
		
		JTAPI void SetEnableSleeping(bool enable);
		JTAPI bool GetIsSleeping() const;
		
		JTAPI bool GetIsKinematic() const;

		/*JTAPI bool SweepTest(std::vector<JoltContactInfo> &contactInfo, const Vector3 &direction, const Vector3 &offsetPosition = Vector3(), const Quaternion &offsetRotation = Quaternion(), float inflation = 0.0f) const;
		JTAPI Quaternion RotationSweepTest(std::vector<JoltContactInfo> &contactInfo, const Quaternion &targetRoation, float stepSize, float sweepSize, const Vector3 &offsetPosition = Vector3(), const Quaternion &offsetRotation = Quaternion()) const;*/

		JTAPI JPH::BodyID *GetJoltActor() const { return _actor; }
		JTAPI JoltShape *GetShape() const { return _shape; }
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
//		void UpdateFromMaterial(BulletMaterial *material) override;
			
	private:
		JoltShape *_shape;
		JPH::BodyID *_actor;
			
		RNDeclareMetaAPI(JoltDynamicBody, JTAPI)
	};
}

#endif /* defined(__RAYNE_JOLTDYNAMICBODY_H_) */
