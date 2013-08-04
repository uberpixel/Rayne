//
//  RBRigidBody.cpp
//  rayne-bullet
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include "RBRigidBody.h"

namespace RN
{
	namespace bullet
	{
		RNDeclareMeta(RigidBody)
		
		RigidBody::RigidBody(Shape *shape, float mass)
		{
			RN_ASSERT(shape, "There already is a shape!");
			
			_shape   = shape->Retain();
			_mass    = mass;
			_inertia = _shape->CalculateLocalInertia(_mass);
		}
		
		RigidBody::RigidBody(Shape *shape, float mass, const Vector3& inertia)
		{
			RN_ASSERT(shape, "There already is a shape!");
			
			_shape   = shape->Retain();
			_mass    = mass;
			_inertia = inertia;
		}
		
		RigidBody::~RigidBody()
		{
			_shape->Release();
			if(_object)
				delete _object;
		}
		
		
		btCollisionObject *RigidBody::CreateCollisionObject()
		{
			btRigidBody::btRigidBodyConstructionInfo info(_mass, this, _shape->bulletShape(), btVector3(_inertia.x, _inertia.y, _inertia.z));
			btRigidBody *rigidBody = new btRigidBody(info);
			
			return rigidBody;
		}
		
		
		void RigidBody::SetMass(float mass)
		{
			Vector3 inertia = _shape->CalculateLocalInertia(mass);			
			SetMass(_mass, inertia);
		}
		
		void RigidBody::SetMass(float mass, const Vector3& inertia)
		{
			_inertia = inertia;
			_mass = mass;
			
			bulletRigidBody()->setMassProps(_mass, btVector3(_inertia.x, _inertia.y, _inertia.z));
		}
		
		
		void RigidBody::ApplyPhysicsMaterial(PhysicsMaterial *material)
		{
			CollisionObject::ApplyPhysicsMaterial(material);
			bulletRigidBody()->setDamping(material->LinearDamping(), material->AngularDamping());
		}
		
		
		void RigidBody::SetCollisionShape(Shape *shape)
		{
			RN_ASSERT(shape, "There already is a shape!");
			
			_shape->Release();
			_shape = shape->Retain();
			
			bulletRigidBody()->setCollisionShape(_shape->bulletShape());
		}
		
		
		
		void RigidBody::SetPosition(const Vector3& position)
		{
			Entity::SetPosition(position);
			
			btTransform transform;
			
			getWorldTransform(transform);
			bulletRigidBody()->setWorldTransform(transform);
		}
		
		void RigidBody::SetRotation(const Quaternion& rotation)
		{
			Entity::SetRotation(rotation);
			
			btTransform transform;
			
			getWorldTransform(transform);
			bulletRigidBody()->setCenterOfMassTransform(transform);
		}
		
		void RigidBody::SetWorldPosition(const Vector3& position)
		{
			Entity::SetWorldPosition(position);
			
			btTransform transform;
			
			getWorldTransform(transform);
			bulletRigidBody()->setWorldTransform(transform);
		}
		
		void RigidBody::SetWorldRotation(const Quaternion& rotation)
		{
			Entity::SetWorldRotation(rotation);
			
			btTransform transform;
			
			getWorldTransform(transform);
			bulletRigidBody()->setCenterOfMassTransform(transform);
		}
		
		
		void RigidBody::SetLinearVelocity(const Vector3& velocity)
		{
			bulletRigidBody()->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
		}
		
		void RigidBody::SetAngularVelocity(const Vector3& velocity)
		{
			bulletRigidBody()->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
		}
		
		
		Vector3 RigidBody::LinearVelocity()
		{
			const btVector3& velocity = bulletRigidBody()->getLinearVelocity();
			return Vector3(velocity.x(), velocity.y(), velocity.z());
		}
		
		Vector3 RigidBody::AngularVelocity()
		{
			const btVector3& velocity = bulletRigidBody()->getAngularVelocity();
			return Vector3(velocity.x(), velocity.y(), velocity.z());
		}
		
		
		void RigidBody::ApplyForce(const Vector3& force)
		{
			bulletRigidBody()->applyCentralForce(btVector3(force.x, force.y, force.z));
		}
		
		void RigidBody::ApplyForce(const Vector3& force, const Vector3& origin)
		{
			bulletRigidBody()->applyForce(btVector3(force.x, force.y, force.z), btVector3(origin.x, origin.y, origin.z));
		}
		
		void RigidBody::ClearForces()
		{
			bulletRigidBody()->clearForces();
		}
		
		void RigidBody::ApplyTorque(const Vector3& torque)
		{
			bulletRigidBody()->applyTorque(btVector3(torque.x, torque.y, torque.z));
		}
		
		void RigidBody::ApplyTorqueImpulse(const Vector3& torque)
		{
			bulletRigidBody()->applyTorqueImpulse(btVector3(torque.x, torque.y, torque.z));
		}
		
		void RigidBody::ApplyImpulse(const Vector3& impulse)
		{
			bulletRigidBody()->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
		}
		
		void RigidBody::ApplyImpulse(const Vector3& impulse, const Vector3& origin)
		{
			bulletRigidBody()->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(origin.x, origin.y, origin.z));
		}
		
		
		void RigidBody::InsertIntoWorld(btDynamicsWorld *world)
		{
			world->addRigidBody(bulletRigidBody());
		}
		
		void RigidBody::RemoveFromWorld(btDynamicsWorld *world)
		{
			world->removeRigidBody(bulletRigidBody());
		}
		
		
		void RigidBody::getWorldTransform(btTransform& worldTrans) const
		{
			const Quaternion& rotation = WorldRotation();
			const Vector3& position = WorldPosition()-_offset;
			
			worldTrans.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
			worldTrans.setOrigin(btVector3(position.x, position.y, position.z));
		}
		
		void RigidBody::setWorldTransform(const btTransform& worldTrans)
		{
			btQuaternion rotation = worldTrans.getRotation();
			btVector3 position = worldTrans.getOrigin();
			
			Entity::SetWorldRotation(Quaternion(rotation.x(), rotation.y(), rotation.z(), rotation.w()));
			Entity::SetWorldPosition(Vector3(position.x(), position.y(), position.z())+_offset);
		}
	}
}
