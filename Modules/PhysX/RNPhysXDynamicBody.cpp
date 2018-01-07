//
//  RNPhysXDynamicBody.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXDynamicBody.h"
#include "RNPhysXWorld.h"
#include "PxPhysicsAPI.h"

namespace RN
{
	RNDefineMeta(PhysXDynamicBody, PhysXCollisionObject)
		
		PhysXDynamicBody::PhysXDynamicBody(PhysXShape *shape, float mass) :
		_shape(shape->Retain()),
		_actor(nullptr),
		_didUpdatePosition(false)
	{
		physx::PxPhysics *physics = PhysXWorld::GetSharedInstance()->GetPhysXInstance();
		_actor = physics->createRigidDynamic(physx::PxTransform(physx::PxIdentity));

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
		
		physx::PxRigidBodyExt::updateMassAndInertia(*_actor, mass);

		_actor->userData = this;
		_actor->setContactReportThreshold(0.0f);

		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		scene->addActor(*_actor);
	}
		
	PhysXDynamicBody::~PhysXDynamicBody()
	{
		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		scene->removeActor(*_actor);
		_actor->release();
		_shape->Release();
	}

	void PhysXDynamicBody::SetCollisionFilter(uint32 group, uint32 mask)
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
	
		
	PhysXDynamicBody *PhysXDynamicBody::WithShape(PhysXShape *shape, float mass)
	{
		PhysXDynamicBody *body = new PhysXDynamicBody(shape, mass);
		return body->Autorelease();
	}
		
	void PhysXDynamicBody::SetMass(float mass)
	{
		physx::PxRigidBodyExt::updateMassAndInertia(*_actor, mass);
	}

	void PhysXDynamicBody::SetLinearVelocity(const Vector3 &velocity)
	{
		_actor->setLinearVelocity(physx::PxVec3(velocity.x, velocity.y, velocity.z));
	}
	void PhysXDynamicBody::SetAngularVelocity(const Vector3 &velocity)
	{
		_actor->setAngularVelocity(physx::PxVec3(velocity.x, velocity.y, velocity.z));
	}
		
	void PhysXDynamicBody::SetDamping(float linear, float angular)
	{
		_actor->setLinearDamping(linear);
		_actor->setAngularDamping(angular);
		_actor->setMaxAngularVelocity(PX_MAX_F32);
	}

	void PhysXDynamicBody::SetMaxAngularVelocity(float max)
	{
		_actor->setMaxAngularVelocity(max);
	}
		
	Vector3 PhysXDynamicBody::GetLinearVelocity() const
	{
		const physx::PxVec3& velocity = _actor->getLinearVelocity();
		return Vector3(velocity.x, velocity.y, velocity.z);
	}
	Vector3 PhysXDynamicBody::GetAngularVelocity() const
	{
		const physx::PxVec3& velocity = _actor->getAngularVelocity();
		return Vector3(velocity.x, velocity.y, velocity.z);
	}
		
		
/*
	void PhysXDynamicBody::ApplyForce(const Vector3 &force)
	{
		_rigidBody->applyCentralForce(btVector3(force.x, force.y, force.z));
	}
	void PhysXDynamicBody::ApplyForce(const Vector3 &force, const Vector3 &origin)
	{
		_rigidBody->applyForce(btVector3(force.x, force.y, force.z), btVector3(origin.x, origin.y, origin.z));
	}
	void PhysXDynamicBody::ClearForces()
	{
		_rigidBody->clearForces();
	}
		
	void PhysXDynamicBody::ApplyTorque(const Vector3 &torque)
	{
		_rigidBody->applyTorque(btVector3(torque.x, torque.y, torque.z));
	}
	void PhysXDynamicBody::ApplyTorqueImpulse(const Vector3 &torque)
	{
		_rigidBody->applyTorqueImpulse(btVector3(torque.x, torque.y, torque.z));
	}
	void PhysXDynamicBody::ApplyImpulse(const Vector3 &impulse)
	{
		_rigidBody->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
	}
	void PhysXDynamicBody::ApplyImpulse(const Vector3 &impulse, const Vector3 &origin)
	{
		_rigidBody->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(origin.x, origin.y, origin.z));
	}*/
		
		
		
	void PhysXDynamicBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		PhysXCollisionObject::DidUpdate(changeSet);
		
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			if(!_didUpdatePosition)
			{
				Vector3 position = GetWorldPosition() - _offset;
				Quaternion rotation = GetWorldRotation();
				_actor->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
			}
			_didUpdatePosition = false;
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

/*	void PhysXDynamicBody::UpdateFromMaterial(BulletMaterial *material)
	{
		_rigidBody->setFriction(material->GetFriction());
		_rigidBody->setRollingFriction(material->GetRollingFriction());
		_rigidBody->setSpinningFriction(material->GetSpinningFriction());
		_rigidBody->setRestitution(material->GetRestitution());
		_rigidBody->setDamping(material->GetLinearDamping(), material->GetAngularDamping());
	}*/

	void PhysXDynamicBody::UpdatePosition()
	{
		if(!_owner)
		{
			return;
		}

		_didUpdatePosition = true;
		const physx::PxTransform &transform = _actor->getGlobalPose();
		GetParent()->SetWorldPosition(Vector3(transform.p.x, transform.p.y, transform.p.z) + _offset);
		GetParent()->SetWorldRotation(Quaternion(transform.q.x, transform.q.y, transform.q.z, transform.q.w));
	}
}
