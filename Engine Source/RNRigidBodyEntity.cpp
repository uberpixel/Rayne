//
//  RNRigidBodyEntity.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEntity.h"
#include "RNWorld.h"
#include "RNKernel.h"
#include <btBulletDynamicsCommon.h>

namespace RN
{
	RigidBodyEntity::RigidBodyEntity(Shape shape) :
		_size(Vector3(1.0f)),
		_shapeType(shape),
		_centralForce(Vector3(0.0f)),
		_centralImpulse(Vector3(0.0f)),
		_torque(Vector3(0.0f))
	{
		_rigidBodyIsInWorld = false;
		
		InitializeProperties();
		CreateRigidBody();
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
	}
	
	RigidBodyEntity::RigidBodyEntity(Shape shape, const Vector3& size, float mass) :
		_size(size),
		_mass(mass),
		_shapeType(shape),
		_centralForce(Vector3(0.0f)),
		_centralImpulse(Vector3(0.0f)),
		_torque(Vector3(0.0f))
	{
		_rigidBodyIsInWorld = false;
		
		InitializeProperties();
		CreateRigidBody();
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
	}
	
	RigidBodyEntity::~RigidBodyEntity()
	{
		World::SharedInstance()->Physics()->RemoveRigidBody(this);
	}

	void RigidBodyEntity::InitializeProperties()
	{
		_linearDamping = _angularDamping = 0.0f;
		_friction = 0.5f;
		_restitution = 0.0f;
		
		_mass = 1.0f;
		_changes = 0;
		
		_shape = 0;
		_rigidbody = 0;
		_triangleMesh = 0;
	}
	
	
	void RigidBodyEntity::PostUpdate()
	{
		// We don't need to acquire the _physicsLock here because PostUpdate() is called
		// in a single threaded manner. The physics thread did already run at this point.
		
		_position = _cachedTransform.Position();
		_rotation = _cachedTransform.Rotation();
		_didChange = true;
		
		_changes &= ~(PositionChange | RotationChange);
	}
	
	
	void RigidBodyEntity::CreateRigidBody()
	{
		if(_shape)
		{
			delete _shape;
			
			if(_triangleMesh)
			{
				delete _triangleMesh;
				_triangleMesh = 0;
			}
		}
		
		switch(_shapeType)
		{
			case ShapeBox:
				_shape = new btBoxShape(btVector3(_size.x, _size.y, _size.z));
				break;
				
			case ShapeSphere:
				_shape = new btSphereShape(_size.x);
				break;
				
			case ShapeCapsule:
				_shape = new btCapsuleShape(_size.x, _size.y);
				break;
				
			case ShapeMesh:
				_shape = GenerateMeshShape();
				break;
		}
		
		btVector3 inertia;
		_shape->calculateLocalInertia(_mass, inertia);
		
		if(!_rigidbody)
		{
			_cachedTransform = *this;
			_changes &= ~(PositionChange | RotationChange);
			
			btRigidBody::btRigidBodyConstructionInfo info(_mass, this, _shape, inertia);
			_rigidbody = new btRigidBody(info);
		}
		else
		{
			_rigidbody->setCollisionShape(_shape);
		}
	}
	
	void RigidBodyEntity::UpdateRigidBody(btDynamicsWorld *world)
	{
		_physicsLock.Lock();
		
		if(!_rigidbody || _changes & SizeChange)
		{
			CreateRigidBody();
			_changes &= ~SizeChange;
		}
		
		if(_changes & MassChange)
		{
			btVector3 inertia;
			_shape->calculateLocalInertia(_mass, inertia);
			_rigidbody->setMassProps(_mass, inertia);
			
			_changes &= ~MassChange;
		}
		
		if(_changes & PositionChange || _changes & RotationChange)
		{
			btTransform transform;
			
			getWorldTransform(transform);
			_rigidbody->setWorldTransform(transform);
			
			_changes &= ~(PositionChange | RotationChange);
		}
		
		if(_changes & DampingChange)
		{
			_rigidbody->setDamping(_linearDamping, _angularDamping);
			_changes &= ~DampingChange;
		}
		
		if(_changes & FrictionChange)
		{
			_rigidbody->setFriction(_friction);
			_changes &= ~FrictionChange;
		}
		
		if(_changes & RestitutionChange)
		{
			_rigidbody->setRestitution(_restitution);
			_changes &= ~RestitutionChange;
		}
		
		if(_changes & ClearForcesChange)
		{
			_rigidbody->clearForces();
			_changes &= ~ClearForcesChange;
		}
		
		if(_changes & ForceChange)
		{
			_rigidbody->applyCentralForce(btVector3(_centralForce.x, _centralForce.y, _centralForce.z));
			_centralForce = Vector3(0.0f);
			
			for(auto i=_forces.begin(); i!=_forces.end(); i++)
			{
				Vector3 force = std::get<0>(*i);
				Vector3 origin = std::get<1>(*i);
				
				_rigidbody->applyForce(btVector3(force.x, force.y, force.z), btVector3(origin.x, origin.y, origin.z));
			}
			
			_forces.clear();
			_changes &= ~ForceChange;
		}
		
		if(_changes & TorqueChange)
		{
			_rigidbody->applyTorque(btVector3(_torque.x, _torque.y, _torque.z));
			_torque = Vector3(0.0f);
			
			for(auto i=_torqueImpulses.begin(); i!=_torqueImpulses.end(); i++)
			{
				_rigidbody->applyTorqueImpulse(btVector3(i->x, i->y, i->z));
			}
			
			_torqueImpulses.clear();
			_changes &= ~TorqueChange;
		}
		
		if(_changes & ImpulseChange)
		{
			_rigidbody->applyCentralImpulse(btVector3(_centralImpulse.x, _centralImpulse.y, _centralImpulse.z));
			_centralImpulse = Vector3(0.0f);
			
			for(auto i=_impulses.begin(); i!=_impulses.end(); i++)
			{
				Vector3 impulse = std::get<0>(*i);
				Vector3 origin = std::get<1>(*i);
				
				_rigidbody->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(origin.x, origin.y, origin.z));
			}
			
			_impulses.clear();
			_changes &= ~ImpulseChange;
		}
		
		_physicsLock.Unlock();
		
		if(!_rigidBodyIsInWorld)
		{
			world->addRigidBody(_rigidbody);
			_rigidBodyIsInWorld = true;
		}
	}
	
	// Physics property setter
	void RigidBodyEntity::SetMass(float mass)
	{
		_physicsLock.Lock();
		
		_mass = mass;
		_changes |= MassChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}
	
	void RigidBodyEntity::SetSize(const Vector3& size)
	{
		_physicsLock.Lock();
		
		_size = size;
		_changes |= SizeChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}
	
	void RigidBodyEntity::SetDamping(float linear, float angular)
	{
		_physicsLock.Lock();
		
		_linearDamping = linear;
		_angularDamping = angular;
		
		_changes |= DampingChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}
	
	void RigidBodyEntity::SetFriction(float friction)
	{
		_physicsLock.Lock();
		
		_friction = friction;		
		_changes |= FrictionChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}
	
	void RigidBodyEntity::SetRestitution(float restitution)
	{
		_physicsLock.Lock();
		
		_restitution = restitution;
		_changes |= RestitutionChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}
	
	void RigidBodyEntity::SetPosition(const Vector3& pos)
	{
		Transform::SetPosition(pos);
		
		_physicsLock.Lock();
		
		_cachedTransform.SetPosition(pos);
		_changes |= PositionChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}
	
	void RigidBodyEntity::SetRotation(const Quaternion& rot)
	{
		Transform::SetRotation(rot);
		
		_physicsLock.Lock();
		
		_cachedTransform.SetRotation(rot);
		_changes |= RotationChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}
	
	void RigidBodyEntity::ApplyForce(const Vector3& force)
	{
		_physicsLock.Lock();
		
		_centralForce += force;
		_changes |= ForceChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}
	
	void RigidBodyEntity::ApplyForce(const Vector3& force, const Vector3& origin)
	{
		_physicsLock.Lock();
		
		_forces.push_back(std::tuple<Vector3, Vector3>(force, origin));
		_changes |= ForceChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}
	
	void RigidBodyEntity::ClearForces()
	{
		_physicsLock.Lock();
		
		_centralForce = Vector3(0.0f);
		_forces.clear();
		
		_changes |= ClearForcesChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}
	
	void RigidBodyEntity::ApplyTorque(const Vector3& torque)
	{
		_physicsLock.Lock();
		
		_torque += torque;
		_changes |= TorqueChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}
	
	void RigidBodyEntity::ApplyTorqueImpulse(const Vector3& torque)
	{
		_physicsLock.Lock();
		
		_torqueImpulses.push_back(torque);
		_changes |= TorqueChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}
	
	void RigidBodyEntity::ApplyImpulse(const Vector3& impulse)
	{
		_physicsLock.Lock();
		
		_centralImpulse += impulse;
		_changes |= ImpulseChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}
	
	void RigidBodyEntity::ApplyImpulse(const Vector3& impulse, const Vector3& origin)
	{
		_physicsLock.Lock();
		
		_impulses.push_back(std::tuple<Vector3, Vector3>(impulse, origin));
		_changes |= ImpulseChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
		_physicsLock.Unlock();
	}

	// Bullet helper
	
	void RigidBodyEntity::getWorldTransform(btTransform& worldTrans) const
	{
		const Quaternion& rot = _cachedTransform.Rotation();
		const Vector3& pos = _cachedTransform.Position();
		
		worldTrans.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
		worldTrans.setOrigin(btVector3(pos.x, pos.y, pos.z));
	}
	
	void RigidBodyEntity::setWorldTransform(const btTransform& worldTrans)
	{
		btQuaternion rot = worldTrans.getRotation();
		btVector3 pos = worldTrans.getOrigin();
		
		_physicsLock.Lock();
		
		_cachedTransform.SetRotation(Quaternion(rot.x(), rot.y(), rot.z(), rot.w()));
		_cachedTransform.SetPosition(Vector3(pos.x(), pos.y(), pos.z()));
		
		_physicsLock.Unlock();
	}
	
	btCollisionShape *RigidBodyEntity::GenerateMeshShape()
	{
		_triangleMesh = new btTriangleMesh();
//		_triangleMesh->addTriangle(btVector3(vert1.x, vert1.y, vert1.z), btVector3(vert2.x, vert2.y, vert2.z), btVector3(vert3.x, vert3.y, vert3.z));
		
		return new btBvhTriangleMeshShape(_triangleMesh, true);
	}
}
