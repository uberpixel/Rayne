//
//  RNPhysXRigidBody.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXRigidBody.h"
#include "RNPhysXWorld.h"
#include "RNPhysXInternals.h"

namespace RN
{
	RNDefineMeta(PhysXRigidBody, SceneNodeAttachment)
		
		PhysXRigidBody::PhysXRigidBody(BulletShape *shape, float mass) :
		_shape(shape->Retain()),
		_rigidBody(nullptr),
		_motionState(new BulletRigidBodyMotionState())
	{
		Vector3 inertia = _shape->CalculateLocalInertia(mass);
		btVector3 btInertia = btVector3(inertia.x, inertia.y, inertia.z);
			
		btRigidBody::btRigidBodyConstructionInfo info(mass, _motionState, _shape->GetBulletShape(), btInertia);
			
		_rigidBody = new btRigidBody(info);
		_rigidBody->setUserPointer(this);
	}
		
	PhysXRigidBody::~PhysXRigidBody()
	{
		_shape->Release();
		delete _rigidBody;
	}
	
/*		
	PhysXRigidBody *PhysXRigidBody::WithShape(BulletShape *shape, float mass)
	{
		BulletRigidBody *body = new BulletRigidBody(shape, mass);
		return body->Autorelease();
	}
	PhysXRigidBody *PhysXRigidBody::WithShapeAndInertia(BulletShape *shape, float mass, const Vector3 &inertia)
	{
		BulletRigidBody *body = new BulletRigidBody(shape, mass, inertia);
		return body->Autorelease();
	}
	
	btCollisionObject *PhysXRigidBody::GetBulletCollisionObject() const
	{
		return _rigidBody;
	}
		
	void PhysXRigidBody::SetMass(float mass)
	{
		Vector3 inertia = _shape->CalculateLocalInertia(mass);
		SetMass(mass, inertia);
	}
	void PhysXRigidBody::SetMass(float mass, const Vector3 &inertia)
	{
		_rigidBody->setMassProps(mass, btVector3(inertia.x, inertia.y, inertia.z));
	}
	void PhysXRigidBody::SetLinearVelocity(const Vector3 &velocity)
	{
		_rigidBody->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
	}
	void PhysXRigidBody::SetAngularVelocity(const Vector3 &velocity)
	{
		_rigidBody->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
	}
	void BulletRigidBody::SetCCDMotionThreshold(float threshold)
	{
		_rigidBody->setCcdMotionThreshold(threshold);
	}
	void BulletRigidBody::SetCCDSweptSphereRadius(float radius)
	{
		_rigidBody->setCcdSweptSphereRadius(radius);
	}
		
	void BulletRigidBody::SetGravity(const RN::Vector3 &gravity)
	{
		_rigidBody->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
	}
		
	void BulletRigidBody::SetDamping(float linear, float angular)
	{
		_rigidBody->setDamping(linear, angular);
	}

	void BulletRigidBody::SetAllowDeactivation(bool canDeactivate)
	{
		_rigidBody->forceActivationState(canDeactivate?ACTIVE_TAG:DISABLE_DEACTIVATION);
	}
		
	Vector3 BulletRigidBody::GetLinearVelocity() const
	{
		const btVector3& velocity = _rigidBody->getLinearVelocity();
		return Vector3(velocity.x(), velocity.y(), velocity.z());
	}
	Vector3 BulletRigidBody::GetAngularVelocity() const
	{
		const btVector3& velocity = _rigidBody->getAngularVelocity();
		return Vector3(velocity.x(), velocity.y(), velocity.z());
	}
		
		
	Vector3 BulletRigidBody::GetCenterOfMass() const
	{
		const btVector3& center = _rigidBody->getCenterOfMassPosition();
		return Vector3(center.x(), center.y(), center.z());
	}
	Matrix BulletRigidBody::GetCenterOfMassTransform() const
	{
		const btTransform& transform = _rigidBody->getCenterOfMassTransform();
			
		btQuaternion rotation = transform.getRotation();
		btVector3 position    = transform.getOrigin();
			
		Matrix matrix;
			
		matrix.Translate(Vector3(position.x(), position.y(), position.z()));
		matrix.Rotate(Quaternion(rotation.x(), rotation.y(), rotation.z(), rotation.w()));
			
		return matrix;
	}
		
		
	void BulletRigidBody::ApplyForce(const Vector3 &force)
	{
		_rigidBody->applyCentralForce(btVector3(force.x, force.y, force.z));
	}
	void BulletRigidBody::ApplyForce(const Vector3 &force, const Vector3 &origin)
	{
		_rigidBody->applyForce(btVector3(force.x, force.y, force.z), btVector3(origin.x, origin.y, origin.z));
	}
	void BulletRigidBody::ClearForces()
	{
		_rigidBody->clearForces();
	}
		
	void BulletRigidBody::ApplyTorque(const Vector3 &torque)
	{
		_rigidBody->applyTorque(btVector3(torque.x, torque.y, torque.z));
	}
	void BulletRigidBody::ApplyTorqueImpulse(const Vector3 &torque)
	{
		_rigidBody->applyTorqueImpulse(btVector3(torque.x, torque.y, torque.z));
	}
	void BulletRigidBody::ApplyImpulse(const Vector3 &impulse)
	{
		_rigidBody->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
	}
	void BulletRigidBody::ApplyImpulse(const Vector3 &impulse, const Vector3 &origin)
	{
		_rigidBody->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(origin.x, origin.y, origin.z));
	}
		
		
		
	void BulletRigidBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		BulletCollisionObject::DidUpdate(changeSet);
			
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			btTransform transform;
				
			_motionState->getWorldTransform(transform);
			_rigidBody->setCenterOfMassTransform(transform);
		}
	}
	void BulletRigidBody::UpdateFromMaterial(BulletMaterial *material)
	{
		_rigidBody->setFriction(material->GetFriction());
		_rigidBody->setRollingFriction(material->GetRollingFriction());
		_rigidBody->setSpinningFriction(material->GetSpinningFriction());
		_rigidBody->setRestitution(material->GetRestitution());
		_rigidBody->setDamping(material->GetLinearDamping(), material->GetAngularDamping());
	}
		
		
	void BulletRigidBody::InsertIntoWorld(BulletWorld *world)
	{
		BulletCollisionObject::InsertIntoWorld(world);
		_motionState->SetSceneNode(this);
			
		{
			btTransform transform;
				
			_motionState->getWorldTransform(transform);
			_rigidBody->setCenterOfMassTransform(transform);
		}
			
		auto bulletWorld = world->GetBulletDynamicsWorld();
		bulletWorld->addRigidBody(_rigidBody, GetCollisionFilter(), GetCollisionFilterMask());
	}
		
	void BulletRigidBody::RemoveFromWorld(BulletWorld *world)
	{
		BulletCollisionObject::RemoveFromWorld(world);
			
		auto bulletWorld = world->GetBulletDynamicsWorld();
		bulletWorld->removeRigidBody(_rigidBody);
	}

	void BulletRigidBody::SetPositionOffset(RN::Vector3 offset)
	{
		BulletCollisionObject::SetPositionOffset(offset);
		_motionState->SetPositionOffset(offset);
	}*/
}