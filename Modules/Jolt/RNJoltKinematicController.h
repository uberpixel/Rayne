//
//  RNJoltKinematicController.h
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLTKINEMATICCONTROLLER_H_
#define __RAYNE_JOLTKINEMATICCONTROLLER_H_

#include "RNJolt.h"
#include "RNJoltCollisionObject.h"
#include "RNJoltShape.h"

namespace JPH
{
	class CharacterVirtual;
}

namespace RN
{
	class JoltCharacterInternals;
	class JoltKinematicController : public JoltCollisionObject
	{
	public:
		JTAPI JoltKinematicController(float radius, float height, float stepOffset = 0.5f, JoltWorld *world = nullptr);
		JTAPI ~JoltKinematicController() override;

		JTAPI void UpdatePosition() override;

		// Gravity is only a solver hint for contacts, floor sticking and stairs.
		// The caller must include gravity integration in the velocity vector.
		JTAPI void Move(const Vector3 &velocity, const Vector3 &gravity, float delta);
		JTAPI std::vector<JoltContactInfo> SweepTestAll(const Vector3 &direction, const Vector3 &offset = Vector3()) const;
		JTAPI JoltContactInfo SweepTest(const Vector3 &direction, const Vector3 &offset = Vector3()) const;
		JTAPI JoltContactInfo OverlapTest() const;
		JTAPI std::vector<JoltContactInfo> OverlapTestAll() const;

		JTAPI bool Resize(float height, bool checkIfBlocked = true);

		JTAPI void SetCollisionFilter(uint32 group, uint32 mask) override;
		JTAPI Vector3 GetFeetOffset() const;
		
		JoltShape *GetShape() const { return _shape; }

		SceneNode *GetObjectBelow() const { return _objectBelow; }
		bool GetIsFalling() const { return _isFalling; }

	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;

		JoltShape *_shape;
		JPH::CharacterVirtual *_controller;

		PIMPL<JoltCharacterInternals> _internals;

		float _radius;
		float _height;
		float _stepOffset;
		SceneNode *_objectBelow;
		bool _isFalling;

		void UpdateGroundState();

		RNDeclareMetaAPI(JoltKinematicController, JTAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_JOLTKINEMATICCONTROLLER_H_) */
