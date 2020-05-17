//
//  RNPhysXKinematicController.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXKinematicController.h"
#include "RNPhysXWorld.h"
#include "RNPhysXInternals.h"

#include "PxPhysicsAPI.h"

namespace RN
{
	RNDefineMeta(PhysXKinematicController, PhysXCollisionObject)
		
	PhysXKinematicController::PhysXKinematicController(float radius, float height, PhysXMaterial *material, float stepOffset) : _gravity(0.0f)
	{
		_material = material->Retain();

		_callback = new PhysXKinematicControllerCallback();

		physx::PxCapsuleControllerDesc desc;
		desc.height = height;
		desc.radius = radius;
		desc.position.set(0.0f, 10.0, 0.0f);
		desc.stepOffset = stepOffset;
		desc.material = _material->GetPhysXMaterial();
		desc.reportCallback = _callback;
		desc.userData = this;

		physx::PxControllerManager *manager = PhysXWorld::GetSharedInstance()->GetPhysXControllerManager();
		_controller = static_cast<physx::PxCapsuleController*>(manager->createController(desc));
	}
		
	PhysXKinematicController::~PhysXKinematicController()
	{
		_controller->release();
		if(_callback) delete _callback;
	}
		
	void PhysXKinematicController::Move(const Vector3 &direction, float delta)
	{
		if(delta < k::EpsilonFloat || direction.GetLength() < k::EpsilonFloat)
		{
			return;
		}

		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterMask;
		physx::PxControllerFilters controllerFilter(&filterData, nullptr, _callback);
		physx::PxControllerCollisionFlags collisionFlags = _controller->move(physx::PxVec3(direction.x, direction.y, direction.z), 0.0f, delta, controllerFilter);

		UpdatePosition();
	}

	void PhysXKinematicController::Gravity(float gforce, float delta)
	{
		float groundDistance = SweepTest(RN::Vector3(0.0f, -10000.0f, 0.0f));
		groundDistance += GetFeetOffset().y+_controller->getContactOffset()*2.0f;
		if(groundDistance < 0.4f && _gravity < k::EpsilonFloat)
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
		{
			Move(RN::Vector3(0.0f, -groundDistance, 0.0f), delta);
		}
	}

	float PhysXKinematicController::SweepTest(const Vector3 &direction, const Vector3 &offset) const
	{
		const physx::PxExtendedVec3 &position = _controller->getPosition();
		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		float length = direction.GetLength();
		Vector3 normalizedDirection = direction.GetNormalized();
		physx::PxSweepBuffer hit;
		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterMask;
		physx::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		scene->sweep(shape->getGeometry().any(), physx::PxTransform(physx::PxVec3(position.x + offset.x, position.y + offset.y, position.z + offset.z)), physx::PxVec3(normalizedDirection.x, normalizedDirection.y, normalizedDirection.z), length, hit, physx::PxHitFlags(physx::PxHitFlag::eDEFAULT), physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eDYNAMIC|physx::PxQueryFlag::eSTATIC));
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, true);

		if(hit.getNbAnyHits() == 0)
			return -1.0f;

		return hit.getAnyHit(0).distance;
	}

	bool PhysXKinematicController::Resize(float height, bool checkIfBlocked)
	{
		bool isBlocked = false;
		float oldHeight = _controller->getHeight();
		if(checkIfBlocked && height > oldHeight)
		{
			float radius = _controller->getRadius();
			float dh = std::max(height - oldHeight - 2.0f * radius, 0.0f);
			physx::PxCapsuleGeometry geom(radius, dh * 0.5f);
			float temporaryShapeHeight = dh + radius * 2.0f;
			float offset = height - temporaryShapeHeight * 0.5f;

			physx::PxExtendedVec3 position = _controller->getPosition();
			physx::PxVec3 pos((float)position.x, (float)position.y + offset, (float)position.z);
			physx::PxQuat orientation(k::Pi_2, physx::PxVec3(0.0f, 0.0f, 1.0f));

			physx::PxFilterData filterData;
			filterData.word0 = GetCollisionFilterMask();
			filterData.word1 = GetCollisionFilterGroup();
			physx::PxOverlapBuffer hit;
			PhysXWorld *physXWorld = PhysXWorld::GetSharedInstance();
			if(physXWorld->GetPhysXScene()->overlap(geom, physx::PxTransform(pos, orientation), hit, physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eANY_HIT|physx::PxQueryFlag::eSTATIC|physx::PxQueryFlag::eDYNAMIC))) isBlocked = true;
		}
		
		if(!isBlocked)
		{
			_controller->resize(height);
		}
		
		return !isBlocked;
	}

	void PhysXKinematicController::SetCollisionFilter(uint32 group, uint32 mask)
	{
		PhysXCollisionObject::SetCollisionFilter(group, mask);

		physx::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);

		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		shape->setSimulationFilterData(filterData);
		shape->setQueryFilterData(filterData);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);

//		_controller->invalidateCache();
	}

	Vector3 PhysXKinematicController::GetFeetOffset() const
	{
		physx::PxVec3 offset = _controller->getFootPosition() - _controller->getPosition();
		return Vector3(offset.x, offset.y, offset.z);
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
		
	void PhysXKinematicController::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		PhysXCollisionObject::DidUpdate(changeSet);
			
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			Vector3 position = GetParent()->GetWorldPosition() - _positionOffset;
			_controller->setPosition(physx::PxExtendedVec3(position.x, position.y, position.z));
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent())
			{
				Vector3 position = GetParent()->GetWorldPosition() - _positionOffset;
				_controller->setPosition(physx::PxExtendedVec3(position.x, position.y, position.z));
			}

			_owner = GetParent();
		}
	}

	void PhysXKinematicController::UpdatePosition()
	{
		if(!_owner)
		{
			return;
		}

		const physx::PxExtendedVec3 &position = _controller->getPosition();
		SetWorldPosition(Vector3(position.x, position.y, position.z) + _positionOffset);
	}
}
