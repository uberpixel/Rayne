//
//  RNPhysXStaticBody.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXStaticBody.h"
#include "RNPhysXWorld.h"
#include "PxPhysicsAPI.h"

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

		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		scene->addActor(*_actor);
	}
		
	PhysXStaticBody::~PhysXStaticBody()
	{
		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		scene->removeActor(*_actor);

		_actor->release();
		_shape->Release();
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

		if (_shape->IsKindOfClass(PhysXCompoundShape::GetMetaClass()))
		{
			PhysXCompoundShape *compound = _shape->Downcast<PhysXCompoundShape>();
			for (PhysXShape *tempShape : compound->_shapes)
			{
				tempShape->GetPhysXShape()->setSimulationFilterData(filterData);
				tempShape->GetPhysXShape()->setQueryFilterData(filterData);
			}
		}
		else
		{
			_shape->GetPhysXShape()->setSimulationFilterData(filterData);
			_shape->GetPhysXShape()->setQueryFilterData(filterData);
		}
	}
	
	void PhysXStaticBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		PhysXCollisionObject::DidUpdate(changeSet);

		if(changeSet & SceneNode::ChangeSet::Position)
		{
			Vector3 position = GetWorldPosition() - _offset;
			Quaternion rotation = GetWorldRotation();
			_actor->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent())
			{
				Vector3 position = GetWorldPosition() - _offset;
				Quaternion rotation = GetWorldRotation();
				_actor->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
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
		SetWorldPosition(Vector3(transform.p.x, transform.p.y, transform.p.z) + _offset);
		SetWorldRotation(Quaternion(transform.q.x, transform.q.y, transform.q.z, transform.q.w));
	}
}
