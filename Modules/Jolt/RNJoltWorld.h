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

#include "RNJoltMaterial.h"
#include "RNJoltShape.h"
#include "RNJoltDynamicBody.h"
#include "RNJoltStaticBody.h"
#include "RNJoltConstraint.h"
#include "RNJoltKinematicController.h"

namespace JPH
{
	class PhysicsSystem;
}

namespace RN
{
	struct JoltInternals;

	class JoltWorld : public SceneAttachment
	{
	public:
		friend class JoltKinematicController;

		JTAPI JoltWorld(const Vector3 &gravity = Vector3(0.0f, -9.81f, 0.0f), uint32 maxBodies = 65536, uint32 maxBodyPairs = 65536, uint32 maxContactConstraints = 10240);
		JTAPI ~JoltWorld();

		JTAPI void SetGravity(const Vector3 &gravity);
		JTAPI Vector3 GetGravity();

		JTAPI void Update(float delta) final;
		JTAPI void WillUpdate(float delta) final;
		JTAPI void SetSubsteps(uint8 substeps);
		JTAPI void SetPaused(bool paused);

		JTAPI JoltContactInfo CastRay(const Vector3 &from, const Vector3 &to, uint32 filterGroup = 0xffffffff, uint32 filterMask = 0xffffffff);
		JTAPI JoltContactInfo CastSweep(JoltShape *shape, const Quaternion &rotation, const Vector3 &from, const Vector3 &to, float inflation = 0.0f, uint32 filterGroup = 0xffffffff, uint32 filterMask = 0xffffffff);
		JTAPI std::vector<JoltContactInfo> CheckOverlap(JoltShape *shape, const Vector3 &position, const Quaternion &rotation, float inflation = 0.0f, uint32 filterGroup = 0xffffffff, uint32 filterMask = 0xffffffff);
		
		//Internal utility function, should not be used outside of theis library
		JTAPI uint16 GetObjectLayer(uint32 collisionGroup, uint32 collisionMask, uint8 broadPhaseLayer);

		JTAPI JPH::PhysicsSystem *GetJoltInstance() const { return _physicsSystem; }

		static JoltWorld *GetSharedInstance() { return _sharedInstance; }

	private:
		static JoltWorld *_sharedInstance;

		JPH::PhysicsSystem *_physicsSystem;
		
		PIMPL<JoltInternals> _internals;
		
		bool _isSimulating;
		bool _didUpdate;

		uint8 _substeps;
		bool _paused;

		RNDeclareMetaAPI(JoltWorld, JTAPI)
	};
}


#endif /* __RAYNE_JOLTWORLD_H_ */
