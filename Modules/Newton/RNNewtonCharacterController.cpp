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
		return 1;
	}

/*	float NewtonCharacterController::SweepTestFilter(const NewtonBody* const body, const NewtonCollision* constshapeHit, const float* const hitContact, const float* const hitNormal, long collisionID, void* constuserData, float intersectParam)
	{
		return intersectParam;
	}*/
		
	NewtonCharacterController::NewtonCharacterController(float radius, float height, float stepHeight) : _gravity(0.0f), _stepHeight(stepHeight), _totalHeight(height + radius * 2.0f), _didUpdatePosition(false)
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
		if(groundDistance < k::EpsilonFloat && _gravity < k::EpsilonFloat)
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

		if(groundDistance > 0.0f)
			Move(RN::Vector3(0.0f, -groundDistance, 0.0f), delta);
	}

	float NewtonCharacterController::SweepTest(const Vector3 &direction, const Vector3 &offset) const
	{
		Vector3 startPosition = GetWorldPosition() + offset + Vector3(0.0f, _stepHeight, 0.0f);
		Matrix poseMatrix = /*Matrix::WithRotation(GetWorldRotation())**/Matrix::WithTranslation(startPosition);
		Vector3 targetPosition = GetWorldPosition() + offset + Vector3(0.0f, _stepHeight, 0.0f) + direction;

		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		float params;
		NewtonWorldConvexCast(newtonInstance, poseMatrix.m, &targetPosition.x, _shape->GetNewtonShape(), &params, (void*)this, SweepTestPreFilter, nullptr, 0, 0);

		return direction.GetLength()*params;
	}

	void NewtonCharacterController::SetCollisionFilter(uint32 group, uint32 mask)
	{
		NewtonCollisionObject::SetCollisionFilter(group, mask);

/*		physx::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);

		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		shape->setSimulationFilterData(filterData);
		shape->setQueryFilterData(filterData);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);*/

//		_controller->invalidateCache();
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
			if(!_didUpdatePosition)
			{
/*				Vector3 position = GetWorldPosition() - _offset;
				Quaternion rotation = GetWorldRotation();
				Matrix poseMatrix = Matrix::WithRotation(rotation) * Matrix::WithTranslation(position);
				NewtonBodySetMatrix(_body, poseMatrix.m);*/
			}

			_didUpdatePosition = false;
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
		GetParent()->SetWorldPosition(Vector3(position.x, position.y, position.z) + _offset);*/
	}
}
