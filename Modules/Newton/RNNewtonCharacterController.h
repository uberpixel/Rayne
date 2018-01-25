//
//  RNNewtonCharacterController.h
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_NEWTONCHARACTERCONTROLLER_H_
#define __RAYNE_NEWTONCHARACTERCONTROLLER_H_

#include "RNNewton.h"
#include "RNNewtonCollisionObject.h"

class NewtonBody;

namespace RN
{
	class NewtonCharacterController : public NewtonCollisionObject
	{
	public:
		friend class CharacterControllerManager;

		NDAPI NewtonCharacterController(float radius, float height, float stepHeight);
		NDAPI ~NewtonCharacterController() override;

		NDAPI void UpdatePosition() override;
			
		NDAPI void Move(const Vector3 &direction, float delta);
		NDAPI void Gravity(float gforce, float delta);
		NDAPI float SweepTest(const Vector3 &direction, const Vector3 &offset = Vector3()) const;

		NDAPI void SetCollisionFilter(uint32 group, uint32 mask) override;
		NDAPI Vector3 GetFeetOffset() const;

	/*	PXAPI void SetFallSpeed(float speed);
		PXAPI void SetJumpSpeed(float speed);
		PXAPI void SetMaxJumpHeight(float maxHeight);
		PXAPI void SetMaxSlope(float maxSlope);
		PXAPI void SetGravity(float gravity);*/
			
/*		PXAPI bool IsOnGround();
		PXAPI void Jump();*/
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;

		static unsigned int SweepTestPreFilter(const NewtonBody* const body, const NewtonCollision* const collision, void* const userData);
//		static float SweepTestFilter(const NewtonBody* const body, const NewtonCollision* constshapeHit, const float* const hitContact, const float* const hitNormal, long collisionID, void* constuserData, float intersectParam);

		NewtonShape *_shape;
		NewtonBody *_body;

		float _gravity;
		float _stepHeight;
		float _totalHeight;
		bool _didUpdatePosition;
			
		RNDeclareMetaAPI(NewtonCharacterController, NDAPI)
	};
}

#endif /* defined(__RAYNE_NEWTONCHARACTERCONTROLLER_H_) */
