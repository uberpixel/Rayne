//
//  RNNewtonCharacterController.cpp
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNNewtonCharacterController.h"
#include "RNNewtonWorld.h"

#include "Newton.h"

namespace RN
{
	RNDefineMeta(NewtonCharacterController, NewtonCollisionObject)

	unsigned int NewtonCharacterController::SweepTestPreFilter(const NewtonBody* const body, const NewtonCollision* const collision, void* const userData)
	{
		NewtonCollisionObject *object0 = static_cast<NewtonCollisionObject*>(NewtonBodyGetUserData(body));
		NewtonCollisionObject *object1 = static_cast<NewtonCollisionObject*>(userData);

		bool filterMask = (object0->GetCollisionFilterGroup() & object1->GetCollisionFilterMask()) && (object1->GetCollisionFilterGroup() & object0->GetCollisionFilterMask());
		bool filterID = (object0->GetCollisionFilterIgnoreID() == 0 && object1->GetCollisionFilterIgnoreID() == 0) || (object0->GetCollisionFilterID() != object1->GetCollisionFilterIgnoreID() && object0->GetCollisionFilterIgnoreID() != object1->GetCollisionFilterID());
		if(filterMask && filterID)
			return 1;

		return 0;
	}

/*	float NewtonCharacterController::SweepTestFilter(const NewtonBody* const body, const NewtonCollision* constshapeHit, const float* const hitContact, const float* const hitNormal, long collisionID, void* constuserData, float intersectParam)
	{
		return intersectParam;
	}*/
		
	NewtonCharacterController::NewtonCharacterController(float radius, float height, float stepHeight) : _gravity(0.0f), _stepHeight(stepHeight), _totalHeight(height + radius * 2.0f)
	{
		RN_ASSERT(height > stepHeight, "Height needs to be bigger than step height!");
		_shape = NewtonCapsuleShape::WithRadius(radius, height - stepHeight)->Retain();

/*		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		_body = NewtonCreateKinematicBody(newtonInstance, _shape->GetNewtonShape(), nullptr);*/
	}
		
	NewtonCharacterController::~NewtonCharacterController()
	{
//		NewtonDestroyBody(_body);
		_shape->Release();
	}
		
	void NewtonCharacterController::Move(const Vector3 &direction, float delta)
	{
		if(delta < k::EpsilonFloat)
		{
			return;
		}

		SetWorldPosition(GetWorldPosition() + direction);

		UpdatePosition();
	}

	void NewtonCharacterController::Gravity(float gforce, float delta)
	{
		float groundDistance = SweepTest(RN::Vector3(0.0f, -1000.0f, 0.0f));
		groundDistance -= _stepHeight;
		if(groundDistance < 0.0f && _gravity < k::EpsilonFloat)
		{
			_gravity = 0.0f;
		}
		else
		{
			_gravity += gforce*delta;
			if(_gravity < groundDistance)
			{
				groundDistance = -_gravity*delta;
			}
			else
			{
				_gravity = 0.0f;
			}
		}

		Move(RN::Vector3(0.0f, -groundDistance, 0.0f), delta);
	}

	float NewtonCharacterController::SweepTest(const Vector3 &direction, const Vector3 &offset) const
	{
		Vector3 startPosition = GetWorldPosition() + offset + Vector3(0.0f, _stepHeight, 0.0f);
		Matrix poseMatrix = Matrix::WithTranslation(startPosition) * Matrix::WithRotation(GetWorldRotation());
		Vector3 targetPosition = GetWorldPosition() + offset + Vector3(0.0f, _stepHeight, 0.0f) + direction;

		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		float params;
		NewtonWorldConvexCast(newtonInstance, poseMatrix.m, &targetPosition.x, _shape->GetNewtonShape(), &params, (void*)this, SweepTestPreFilter, nullptr, 0, 0);

		return direction.GetLength()*params;
	}

	Vector3 NewtonCharacterController::GetFeetOffset() const
	{
		return RN::Vector3(0.0f, -_totalHeight/2.0, 0.0f);
	}

/*	void BulletKinematicController::SetFallSpeed(float speed)
	{
		_controller->setFallSpeed(speed);
	}
	void BulletKinematicController::SetJumpSpeed(float speed)
	{
		_controller->setJumpSpeed(speed);
	}
	void BulletKinematicController::SetMaxJumpHeight(float maxHeight)
	{
		_controller->setMaxJumpHeight(maxHeight);
	}
	void BulletKinematicController::SetMaxSlope(float maxSlope)
	{
		_controller->setMaxSlope(maxSlope);
	}
	void BulletKinematicController::SetGravity(float gravity)
	{
		_controller->setGravity(btVector3(0.0f, gravity, 0.0f));
	}
		
		
	bool BulletKinematicController::IsOnGround()
	{
		return _controller->canJump();
	}
	void BulletKinematicController::Jump()
	{
		_controller->jump();
	}*/
		
	void NewtonCharacterController::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		NewtonCollisionObject::DidUpdate(changeSet);
			
		if(changeSet & SceneNode::ChangeSet::Position)
		{
/*			Vector3 position = GetWorldPosition() - _offset;
			Quaternion rotation = GetWorldRotation();
			Matrix poseMatrix = Matrix::WithRotation(rotation) * Matrix::WithTranslation(position);
			NewtonBodySetMatrix(_body, poseMatrix.m);*/
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent())
			{
/*				Vector3 position = GetWorldPosition() - _offset;
				Quaternion rotation = GetWorldRotation();
				Matrix poseMatrix = Matrix::WithRotation(rotation) * Matrix::WithTranslation(position);
				NewtonBodySetMatrix(_body, poseMatrix.m);*/
			}

			_owner = GetParent();
		}
	}

	void NewtonCharacterController::UpdatePosition()
	{
		if(!_owner)
		{
			return;
		}

/*		const physx::PxExtendedVec3 &position = _controller->getPosition();

		_didUpdatePosition = true;
		SetWorldPosition(Vector3(position.x, position.y, position.z) + _offset);*/
	}
}
