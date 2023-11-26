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
	class JoltKinematicControllerCallback;
	class JoltKinematicController : public JoltCollisionObject
	{
	public:
		JTAPI JoltKinematicController(float radius, float height, JoltMaterial *material, float stepOffset = 0.5f);
		JTAPI ~JoltKinematicController() override;

		JTAPI void UpdatePosition() override;
			
		JTAPI void Move(const Vector3 &direction, float delta);
		JTAPI void Gravity(float gforce, float delta);
		JTAPI std::vector<JoltContactInfo> SweepTestAll(const Vector3 &direction, const Vector3 &offset = Vector3()) const;
		JTAPI JoltContactInfo SweepTest(const Vector3 &direction, const Vector3 &offset = Vector3()) const;
		JTAPI JoltContactInfo OverlapTest() const;
		JTAPI std::vector<JoltContactInfo> OverlapTestAll() const;
		
		JTAPI bool Resize(float height, bool checkIfBlocked = true);

		JTAPI void SetCollisionFilter(uint32 group, uint32 mask) override;
		JTAPI Vector3 GetFeetOffset() const;
		
		SceneNode *GetObjectBelow() const { return _objectBelow; }
		bool GetIsFalling() const { return _isFalling; }

		JTAPI void Jump(float force);

	/*	JTAPI void SetFallSpeed(float speed);
		JTAPI void SetJumpSpeed(float speed);
		JTAPI void SetMaxJumpHeight(float maxHeight);
		JTAPI void SetMaxSlope(float maxSlope);
		JTAPI void SetGravity(float gravity);*/
			
/*		JTAPI bool IsOnGround();
		JTAPI void Jump();*/
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
			
		//Jolt::PxCapsuleController *_controller;
		//JoltMaterial *_material;
		
		JoltShape *_shape;
		JPH::CharacterVirtual *_controller;

		//JoltKinematicControllerCallback *_callback;

		float _fallSpeed;
		SceneNode *_objectBelow;
		bool _isFalling;
			
		RNDeclareMetaAPI(JoltKinematicController, JTAPI)
	};
}

#endif /* defined(__RAYNE_JOLTKINEMATICCONTROLLER_H_) */
