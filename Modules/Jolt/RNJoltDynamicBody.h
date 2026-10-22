//
//  RNJoltDynamicBody.h
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLTDYNAMICBODY_H_
#define __RAYNE_JOLTDYNAMICBODY_H_

#include "RNJoltBodyMotion.h"
#include "RNJoltCollisionObject.h"

namespace JPH
{
	class BodyInterface;
	class BodyID;
	class Constraint;
}

namespace RN
{
	class JoltRigidBodyController;
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

		JTAPI JoltDynamicBody(JoltShape *shape, float mass, JoltWorld *world = nullptr);
		JTAPI ~JoltDynamicBody() override;

		JTAPI static JoltDynamicBody *WithShape(JoltShape *shape, float mass, JoltWorld *world = nullptr);

		// Main thread only, outside physics updates and concurrent body access. Frames remain
		// unchanged during the transaction. Absolute velocities include frame motion at the native center of mass;
		// pending forces and torques only rotate. Sleeping bodies wake when converted
		// local velocities require motion. Disabled bodies remain outside simulation.
		// Rejects incompatible velocity limits,
		// locked axes, or static-body motion before callbacks. Allocates
		// every destination body before beforeTransfer removes the accepted constraints.
		// A false result leaves all source bodies and constraints unchanged. The callback
		// must remove accepted game-owned constraints without throwing or changing bodies.
		JTAPI static bool TransferToWorld(const std::vector<JoltDynamicBody *> &bodies, JoltWorld *destination,
			const std::function<bool(const JPH::Constraint *)> &canRemoveConstraint, const std::function<void()> &beforeTransfer, const std::function<void()> &afterTransfer);

		// Also transfers controllers, remapping their support anchors to transferred bodies.
		// Rejects controller support anchors outside this group; native support joints
		// are recreated automatically, while callbacks remove/rebuild game-owned joints.
		// Controller orientation is application-driven; native angular spin stays locked.
		JTAPI static bool TransferToWorld(const std::vector<JoltDynamicBody *> &bodies, const std::vector<JoltRigidBodyController *> &controllers, JoltWorld *destination,
			const std::function<bool(const JPH::Constraint *)> &canRemoveConstraint, const std::function<void()> &beforeTransfer, const std::function<void()> &afterTransfer);

		JTAPI void UpdatePosition() override;

		JTAPI void SetCollisionFilter(uint32 group, uint32 mask) override;

		JTAPI void SetShape(JoltShape *shape, float mass);
		JTAPI void SetShape(JoltShape *shape, float mass, const Vector3 &positionOffset);
		JTAPI void SetMass(float mass);
		JTAPI void RefreshShapeMassProperties(float mass);
		JTAPI void SetLinearVelocity(const Vector3 &velocity);
		JTAPI void SetAngularVelocity(const Vector3 &velocity);
		JTAPI void SetDamping(float linear, float angular);
		JTAPI void SetMaxLinearVelocity(float max);
		JTAPI void SetMaxAngularVelocity(float max);
		JTAPI void SetMaxDepenetrationVelocity(float max);
		JTAPI void SetEnableCCD(bool enable);
		JTAPI void SetEnableGravity(bool enable);
		bool GetIsGravityEnabled() const { return _isGravityEnabled; }
		JTAPI void SetEnableKinematic(bool enable);
		JTAPI void SetEnableSimulation(bool enable);
		JTAPI void LockMovement(uint32 lockFlags);
		JTAPI void SetSolverIterationCount(uint32 positionIterations, uint32 velocityIterations);

		JTAPI void SetFriction(float friction);
		JTAPI void SetRestitution(float restitution);

		JTAPI void SetKinematicTarget(const JoltPosition &globalPosition, const Quaternion &rotation, float delta);
		//JTAPI void AccelerateToTarget(const Vector3 &position, const Quaternion &rotation, float delta);

		JTAPI void AddForce(const Vector3 &force);
		JTAPI void AddForce(const Vector3 &force, const JoltPosition &globalOrigin);
		JTAPI void ApplyGravity(const Vector3 &gravity);
		// Adds a frame acceleration independently of the gravity enable/support settings.
		JTAPI void ApplyAcceleration(const Vector3 &acceleration);
		//		JTAPI void ClearForces();

		JTAPI void AddTorque(const Vector3 &torque);
		JTAPI void AddTorqueImpulse(const Vector3 &torque);
		JTAPI void AddImpulse(const Vector3 &impulse);
		JTAPI void AddImpulse(const Vector3 &impulse, const JoltPosition &globalOrigin);
		JTAPI bool ApplyBuoyancyImpulse(const JoltPosition &globalSurfacePosition, const Vector3 &surfaceNormal, float buoyancy, float linearDrag, float angularDrag, const Vector3 &fluidVelocity, const Vector3 &gravity, float delta);

		JTAPI float GetMass() const;

		JTAPI Vector3 GetLinearVelocity() const;
		JTAPI Vector3 GetAngularVelocity() const;
		JTAPI Vector3 GetPointVelocity(const JoltPosition &globalPosition) const;
		JTAPI JoltPointMotionProperties GetPointMotionProperties(const JoltPosition &globalPosition) const;
		JTAPI JoltPosition GetCenterOfMassPosition() const;
		JTAPI float GetPointImpulseEffectiveMass(const JoltPosition &globalPosition, const Vector3 &direction) const;
		JTAPI float GetAngularImpulseEffectiveInertia(const Vector3 &axis) const;

		JTAPI void SetEnableSleeping(bool enable);
		JTAPI void SetAllowSleeping(bool allow);
		JTAPI bool GetIsSleeping() const;

		JTAPI bool GetIsKinematic() const;
		JTAPI bool GetIsSimulationEnabled() const { return _isInSimulation; }
		/*JTAPI bool SweepTest(std::vector<JoltContactInfo> &contactInfo, const Vector3 &direction, const Vector3 &offsetPosition = Vector3(), const Quaternion &offsetRotation = Quaternion(), float inflation = 0.0f) const;
		JTAPI Quaternion RotationSweepTest(std::vector<JoltContactInfo> &contactInfo, const Quaternion &targetRoation, float stepSize, float sweepSize, const Vector3 &offsetPosition = Vector3(), const Quaternion &offsetRotation = Quaternion()) const;*/

		// Body IDs are only meaningful within GetJoltWorld().
		JTAPI uint32 GetJoltBodyID() const;
		JTAPI JPH::BodyID *GetJoltActor() const { return _actor; }
		JTAPI JoltShape *GetShape() const { return _shape; }

	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		//		void UpdateFromMaterial(BulletMaterial *material) override;

	private:
		JPH::BodyInterface *GetBodyInterfaceIfInSimulation();
		bool RefreshMotionCacheIfNeeded() const;
		void InvalidateMotionCache() const { _motionCacheIsValid = false; }

		JoltShape *_shape;
		JPH::BodyID *_actor;
		float _mass;
		Vector3 _appliedShapeCenterOfMass;
		bool _isKinematic;
		bool _isGravityEnabled;
		bool _isInSimulation;
		mutable bool _motionCacheIsValid = false;
		mutable JoltPointMotionProperties _motionCacheProperties;
		mutable JoltPosition _motionCacheCenterOfMass = JoltPosition();
		mutable Vector3 _motionCacheLinearVelocity = Vector3();
		mutable Vector3 _motionCacheAngularVelocity = Vector3();

		RNDeclareMetaAPI(JoltDynamicBody, JTAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_JOLTDYNAMICBODY_H_) */
