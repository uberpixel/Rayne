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
		
		RigidBody::RigidBody(class Shape *shape, float mass) :
			RigidBody(shape, 0, mass)
		{}
		
		RigidBody::RigidBody(class Shape *shape, float mass, const Vector3& inertia) :
			RigidBody(shape, 0, mass, inertia)
		{}
		
		
		RigidBody::RigidBody(class Shape *shape, PhysicsMaterial *material, float mass)
		{
			RN_ASSERT0(shape);
			
			_shape = shape->Retain();
			_material = 0;
			_rigidBody = 0;
			
			CreateRigidBody(mass, _shape->CalculateLocalInertia(mass));
			SetMaterial(material);
		}
		
		RigidBody::RigidBody(class Shape *shape, PhysicsMaterial *material, float mass, const Vector3& inertia)
		{
			RN_ASSERT0(shape);
			
			_shape = shape->Retain();
			_material = 0;
			_rigidBody = 0;
			
			CreateRigidBody(mass, inertia);
			SetMaterial(material);
		}
		
		RigidBody::~RigidBody()
		{
			_shape->Release();
			
			if(_material)
			{
				_material->RemoveListener(this);
				_material->Release();
			}
			
			delete _rigidBody;
		}
		
		
		void RigidBody::CreateRigidBody(float mass, const Vector3& inertia)
		{
			btRigidBody::btRigidBodyConstructionInfo info(mass, this, _shape->bulletShape(), btVector3(inertia.x, inertia.y, inertia.z));
			_rigidBody = new btRigidBody(info);
		}
		
		void RigidBody::ApplyMaterial()
		{
			_rigidBody->setDamping(_material->LinearDamping(), _material->AngularDamping());
			_rigidBody->setFriction(_material->Friction());
			_rigidBody->setRestitution(_material->Restitution());
		}
		
		
		void RigidBody::SetMass(float mass)
		{
			Vector3 inertia = _shape->CalculateLocalInertia(mass);
			_rigidBody->setMassProps(mass, btVector3(inertia.x, inertia.y, inertia.z));
		}
		
		void RigidBody::SetMass(float mass, const Vector3& inertia)
		{
			_rigidBody->setMassProps(mass, btVector3(inertia.x, inertia.y, inertia.z));
		}
		
		
		
		void RigidBody::SetShape(class Shape *shape)
		{
			RN_ASSERT0(shape);
			
			_shape->Release();
			_shape = shape->Retain();
			
			_rigidBody->setCollisionShape(_shape->bulletShape());
		}
		
		void RigidBody::SetMaterial(PhysicsMaterial *material)
		{
			if(_material)
			{
				_material->RemoveListener(this);
				_material->Release();
			}
			
			_material = material ? material->Retain() : 0;
			
			if(_material)
			{
				_material->AddListener(this, [this](PhysicsMaterial *material) {
					RN_ASSERT0(material == _material);
					ApplyMaterial();
				});
				
				ApplyMaterial();
			}
		}
		
		
		void RigidBody::SetPosition(const Vector3& position)
		{
			Entity::SetPosition(position);
			
			btTransform transform;
			
			getWorldTransform(transform);
			_rigidBody->setWorldTransform(transform);
		}
		
		void RigidBody::SetRotation(const Quaternion& rotation)
		{
			Entity::SetRotation(rotation);
			
			btTransform transform;
			
			getWorldTransform(transform);
			_rigidBody->setWorldTransform(transform);
		}
		
		void RigidBody::SetWorldPosition(const Vector3& position)
		{
			Entity::SetWorldPosition(position);
			
			btTransform transform;
			
			getWorldTransform(transform);
			_rigidBody->setWorldTransform(transform);
		}
		
		void RigidBody::SetWorldRotation(const Quaternion& rotation)
		{
			Entity::SetWorldRotation(rotation);
			
			btTransform transform;
			
			getWorldTransform(transform);
			_rigidBody->setWorldTransform(transform);
		}
		
		
		void RigidBody::SetLinearVelocity(const Vector3& velocity)
		{
			_rigidBody->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
		}
		
		void RigidBody::SetAngularVelocity(const Vector3& velocity)
		{
			_rigidBody->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
		}
		
		void RigidBody::ApplyForce(const Vector3& force)
		{
			_rigidBody->applyCentralForce(btVector3(force.x, force.y, force.z));
		}
		
		void RigidBody::ApplyForce(const Vector3& force, const Vector3& origin)
		{
			_rigidBody->applyForce(btVector3(force.x, force.y, force.z), btVector3(origin.x, origin.y, origin.z));
		}
		
		void RigidBody::ClearForces()
		{
			_rigidBody->clearForces();
		}
		
		void RigidBody::ApplyTorque(const Vector3& torque)
		{
			_rigidBody->applyTorque(btVector3(torque.x, torque.y, torque.z));
		}
		
		void RigidBody::ApplyTorqueImpulse(const Vector3& torque)
		{
			_rigidBody->applyTorqueImpulse(btVector3(torque.x, torque.y, torque.z));
		}
		
		void RigidBody::ApplyImpulse(const Vector3& impulse)
		{
			_rigidBody->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
		}
		
		void RigidBody::ApplyImpulse(const Vector3& impulse, const Vector3& origin)
		{
			_rigidBody->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(origin.x, origin.y, origin.z));
		}
		
		
		
		void RigidBody::getWorldTransform(btTransform& worldTrans)
		{
			const Quaternion& rotation = WorldRotation();
			const Vector3& position = WorldPosition();
			
			worldTrans.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
			worldTrans.setOrigin(btVector3(position.x, position.y, position.z));
		}
		
		void RigidBody::setWorldTransform(const btTransform& worldTrans)
		{
			btQuaternion rotation = worldTrans.getRotation();
			btVector3 position = worldTrans.getOrigin();
			
			Entity::SetWorldRotation(Quaternion(rotation.x(), rotation.y(), rotation.z(), rotation.w()));
			Entity::SetWorldPosition(Vector3(position.x(), position.y(), position.z()));
		}
	}
}
