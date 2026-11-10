//
//  RNPhysXStaticBody.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXStaticBody.h"
#include "PxPhysicsAPI.h"
#include "RNPhysXWorld.h"

namespace RN
{
	RNDefineMeta(PhysXStaticBody, PhysXCollisionObject)

	PhysXStaticBody::PhysXStaticBody(PhysXShape *shape) :
		_shape(shape->Retain()),
		_actor(nullptr)
	{
		physx::PxPhysics *physics = PhysXWorld::GetSharedInstance()->GetPhysXInstance();
		_actor = physics->createRigidStatic(physx::PxTransform(physx::PxIdentity));

		if(shape->IsKindOfClass(PhysXCompoundShape::GetMetaClass()))
		{
			PhysXCompoundShape *compound = shape->Downcast<PhysXCompoundShape>();
			for(PhysXShape *tempShape : compound->_shapes)
			{
				_actor->attachShape(*tempShape->GetPhysXShape());
			}
		}
		else
		{
			_actor->attachShape(*shape->GetPhysXShape());
		}

		_actor->userData = this;

		PhysXWorld::GetSharedInstance()->Lock();
		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		scene->addActor(*_actor);
		PhysXWorld::GetSharedInstance()->Unlock();
	}

	PhysXStaticBody::~PhysXStaticBody()
	{
		PhysXWorld *world = PhysXWorld::GetSharedInstance();
		if(world)
		{
			world->Lock();
			physx::PxScene *scene = world->GetPhysXScene();
			scene->removeActor(*_actor);
			world->Unlock();
		}

		_actor->release();
		_shape->Release();
	}

	void PhysXStaticBody::ApplyPose()
	{
		if(!GetParent()) return;

		if(_actor)
		{
			Vector3 positionOffset = GetWorldRotation().GetRotatedVector(_positionOffset);
			Vector3 position = GetWorldPosition() - positionOffset;
			Quaternion rotation = GetWorldRotation() * _rotationOffset;

			_actor->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
		}
	}


	PhysXStaticBody *PhysXStaticBody::WithShape(PhysXShape *shape)
	{
		PhysXStaticBody *body = new PhysXStaticBody(shape);
		return body->Autorelease();
	}

	void PhysXStaticBody::SetCollisionFilter(uint32 group, uint32 mask)
	{
		PhysXCollisionObject::SetCollisionFilter(group, mask);

		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;

		if(_shape->IsKindOfClass(PhysXCompoundShape::GetMetaClass()))
		{
			PhysXCompoundShape *compound = _shape->Downcast<PhysXCompoundShape>();
			for(PhysXShape *tempShape : compound->_shapes)
			{
				PhysXWorld::GetSharedInstance()->Lock();
				tempShape->GetPhysXShape()->setSimulationFilterData(filterData);
				tempShape->GetPhysXShape()->setQueryFilterData(filterData);
				PhysXWorld::GetSharedInstance()->Unlock();
			}
		}
		else
		{
			PhysXWorld::GetSharedInstance()->Lock();
			_shape->GetPhysXShape()->setSimulationFilterData(filterData);
			_shape->GetPhysXShape()->setQueryFilterData(filterData);
			PhysXWorld::GetSharedInstance()->Unlock();
		}
	}

	AABB PhysXStaticBody::GetPhysXAABB(float inflation) const
	{
		if(!_actor) return AABB();

		const physx::PxBounds3 bounds = _actor->getWorldBounds(inflation);
		return AABB(Vector3(bounds.minimum.x, bounds.minimum.y, bounds.minimum.z),
					Vector3(bounds.maximum.x, bounds.maximum.y, bounds.maximum.z));
	}

	void PhysXStaticBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		PhysXCollisionObject::DidUpdate(changeSet);
		PhysXWorld *world = PhysXWorld::GetSharedInstance();

		if(world && (changeSet & SceneNode::ChangeSet::Position))
		{
			world->EnqueuePoseChange(this);
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(world && !_owner && GetParent())
			{
				world->Lock();
				ApplyPose();
				world->Unlock();
			}

			_owner = GetParent();
		}
	}

	void PhysXStaticBody::UpdatePosition()
	{
		if(!_owner)
		{
			return;
		}

		const physx::PxTransform &transform = _actor->getGlobalPose();
		RN::Quaternion rotation = Quaternion(transform.q.x, transform.q.y, transform.q.z, transform.q.w) * _rotationOffset.GetConjugated();
		RN::Vector3 positionOffset = rotation.GetRotatedVector(_positionOffset);
		SetWorldPosition(Vector3(transform.p.x, transform.p.y, transform.p.z) + positionOffset);
		SetWorldRotation(rotation);
	}

	void PhysXStaticBody::GetPhysXTransform(Vector3 &position, Quaternion &rotation) const
	{
		if(_actor)
		{
			const physx::PxTransform &transform = _actor->getGlobalPose();
			rotation = Quaternion(transform.q.x, transform.q.y, transform.q.z, transform.q.w) * _rotationOffset.GetConjugated();

			Vector3 positionOffset = rotation.GetRotatedVector(_positionOffset);
			position = Vector3(transform.p.x, transform.p.y, transform.p.z) + positionOffset;
		}
		else
		{
			position = Vector3();
			rotation = Quaternion();
		}
	}

	physx::PxRigidActor *PhysXStaticBody::GetPhysXRigidActor() const
	{
		return _actor;
	}
} // namespace RN
