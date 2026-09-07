//
//  RNJoltWorld.h
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLTWORLD_H_
#define __RAYNE_JOLTWORLD_H_

#include "RNJolt.h"

#include "RNJoltConstraint.h"
#include "RNJoltDynamicBody.h"
#include "RNJoltKinematicController.h"
#include "RNJoltRigidBodyController.h"
#include "RNJoltShape.h"
#include "RNJoltStaticBody.h"

namespace JPH
{
	class PhysicsSystem;
	class Body;
	class BodyID;
} // namespace JPH

namespace RN
{
	struct JoltInternals;
	class JoltCollisionObject;

	class JoltWorld : public SceneAttachment
	{
	public:
		friend class JoltConstraint;
		friend class JoltDynamicBody;
		friend class JoltKinematicController;
		friend class JoltRigidBodyController;
		friend class JoltStaticBody;

		// workerCount: -1 selects hardware concurrency minus one (or zero if unknown), 0 uses only the calling thread.
		// Positive values add that many workers; the calling thread also executes physics jobs.
		JTAPI JoltWorld(const Vector3 &gravity = Vector3(0.0f, -9.81f, 0.0f), uint32 maxBodies = 65536, uint32 maxBodyPairs = 65536, uint32 maxContactConstraints = 10240, int32 workerCount = -1);
		JTAPI ~JoltWorld();

		JTAPI void SetGravity(const Vector3 &gravity);
		JTAPI Vector3 GetGravity();

		JTAPI void SetWorldTransform(const PositionType &position, const Quaternion &rotation);
		JTAPI void SetWorldPosition(const PositionType &position);
		JTAPI void SetWorldRotation(const Quaternion &rotation);
		JTAPI const PositionType &GetWorldPosition() const { return _worldPosition; }
		JTAPI const Quaternion &GetWorldRotation() const { return _worldRotation; }

		JTAPI JoltPosition ConvertPositionToPhysicsWorld(const PositionType &position) const;
		JTAPI PositionType ConvertPositionFromPhysicsWorld(const JoltPosition &position) const;
		JTAPI Vector3 ConvertVectorToPhysicsWorld(const Vector3 &vector) const;
		JTAPI Vector3 ConvertVectorFromPhysicsWorld(const Vector3 &vector) const;
		JTAPI Quaternion ConvertRotationToPhysicsWorld(const Quaternion &rotation) const;
		JTAPI Quaternion ConvertRotationFromPhysicsWorld(const Quaternion &rotation) const;
		JTAPI void SetDefaultDynamicBodyDamping(float linear, float angular);
		JTAPI void SetDefaultDynamicBodyMaxVelocity(float linear, float angular);
		JTAPI float GetDefaultDynamicBodyLinearDamping() const { return _defaultDynamicBodyLinearDamping; }
		JTAPI float GetDefaultDynamicBodyAngularDamping() const { return _defaultDynamicBodyAngularDamping; }
		JTAPI float GetDefaultDynamicBodyMaxLinearVelocity() const { return _defaultDynamicBodyMaxLinearVelocity; }
		JTAPI float GetDefaultDynamicBodyMaxAngularVelocity() const { return _defaultDynamicBodyMaxAngularVelocity; }

		JTAPI void Update(float delta) final;
		JTAPI void WillUpdate(float delta) final;
		JTAPI void SetSubsteps(uint8 substeps);
		JTAPI void SetSolverIterationCount(uint32 positionIterations, uint32 velocityIterations);
		JTAPI void SetPenetrationSlop(float penetrationSlop);
		JTAPI void SetContactCorrection(float baumgarte, float maxPenetrationDistance);
		JTAPI void SetInternalEdgeRemovalVertexTolerance(float tolerance);
		JTAPI void SetDeterministicSimulation(bool deterministic);
		JTAPI void SetPaused(bool paused);

		JTAPI JoltContactInfo CastRay(const JoltPosition &globalFrom, const JoltPosition &globalTo, uint32 filterGroup = 0xffffffff, uint32 filterMask = 0xffffffff);
		JTAPI JoltContactInfo CastSweep(JoltShape *shape, const Quaternion &rotation, const JoltPosition &globalFrom, const JoltPosition &globalTo, const Vector3 &scale = Vector3(1.0f, 1.0f, 1.0f), uint32 filterGroup = 0xffffffff, uint32 filterMask = 0xffffffff);
		JTAPI std::vector<JoltContactInfo> CheckOverlap(JoltShape *shape, const JoltPosition &globalPosition, const Quaternion &rotation, const Vector3 &scale = Vector3(1.0f, 1.0f, 1.0f), uint32 filterGroup = 0xffffffff, uint32 filterMask = 0xffffffff);

		//Internal utility function, should not be used outside of this library
		JTAPI uint16 GetObjectLayer(uint32 collisionGroup, uint32 collisionMask, uint8 broadPhaseLayer);

		JTAPI JPH::PhysicsSystem *GetJoltInstance() const { return _physicsSystem; }

		//These as well as the body creation should all be called on the same thread!
		JTAPI void PrepareLoadingLevel();
		JTAPI void FinalizeLoadingLevel();
		JTAPI void OptimizeBroadPhase();
		bool IsLoadingLevel() const { return _isLoadingLevel; }
		JTAPI void AddBodyForLoadingLevel(JPH::Body *body);

		JTAPI void EnumerateActiveCollisionObjects(const std::function<void(JoltCollisionObject *)> &callback);

		static JoltWorld *GetSharedInstance() { return _sharedInstance; }

	private:
		static void InitializeWorkerThread(int threadIndex);
		JTAPI void QueueBodyRemoval(const JPH::BodyID &bodyID);
		JTAPI void CancelQueuedBodyRemoval(const JPH::BodyID &bodyID);
		JTAPI void UpdateDynamicBodyPositions();
		JTAPI void SetBodyPairCollisionEnabled(const JPH::BodyID &body1, const JPH::BodyID &body2, bool enabled);
		JTAPI void SetConnectedBodyCollisionFilteringEnabled(const JPH::BodyID &body1, const JPH::BodyID &body2, bool enabled);
		JTAPI void RemoveConstraintsForBody(const JPH::BodyID &bodyID);
		JTAPI void ProcessQueuedBodyRemovals();

		static JoltWorld *_sharedInstance;

		JPH::PhysicsSystem *_physicsSystem;
		float _defaultDynamicBodyLinearDamping;
		float _defaultDynamicBodyAngularDamping;
		float _defaultDynamicBodyMaxLinearVelocity;
		float _defaultDynamicBodyMaxAngularVelocity;
		PositionType _worldPosition;
		Quaternion _worldRotation;
		Quaternion _inverseWorldRotation;
		Quaternion _worldPositionRotation;
		Quaternion _inverseWorldPositionRotation;

		PIMPL<JoltInternals> _internals;

		bool _isLoadingLevel;

		bool _isSimulating;
		bool _didUpdate;

		uint8 _substeps;
		bool _paused;

		RNDeclareMetaAPI(JoltWorld, JTAPI)
	};
} // namespace RN


#endif /* __RAYNE_JOLTWORLD_H_ */
