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
		_shapeType(shape)
	{
		_mass = 1.0f;
		_changes = 0;
		
		_shape = 0;
		_rigidbody = 0;
		_triangleMesh = 0;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
	}
	
	RigidBodyEntity::~RigidBodyEntity()
	{
		World::SharedInstance()->Physics()->RemoveRigidBody(this);
	}
	
	
	void RigidBodyEntity::Update(float delta)
	{
	}
	
	void RigidBodyEntity::PostUpdate()
	{
		_position = _cachedTransform.Position();
		_rotation = _cachedTransform.Rotation();
		_didChange = true;
		
		_changes &= ~PositionChange;
		_changes &= ~RotationChange;
	}
	
	
	void RigidBodyEntity::UpdateRigidBody(btDynamicsWorld *world)
	{		
		if(!_shape || _changes & SizeChange)
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
					
				case ShapeMesh:
					_shape = GenerateMeshShape();
					break;
			}
			
			btVector3 inertia(0, 0, 0);
			_shape->calculateLocalInertia(_mass, inertia);
			
			if(!_rigidbody)
			{
				_transformLock.Lock();
				_cachedTransform = *this;
				_changes &= ~(PositionChange | RotationChange);
				_transformLock.Unlock();
				
				btRigidBody::btRigidBodyConstructionInfo info(_mass, this, _shape, inertia);
				
				_rigidbody = new btRigidBody(info);
				world->addRigidBody(_rigidbody);
			}
			else
			{
				_rigidbody->setCollisionShape(_shape);
			}
			
			_changes &= ~(MassChange | SizeChange);
		}
		
		if(_changes & MassChange)
		{
			btVector3 inertia(0, 0, 0);
			_shape->calculateLocalInertia(_mass, inertia);
			_rigidbody->setMassProps(_mass, inertia);
			
			_changes &= ~MassChange;
		}
		
		if(_changes & PositionChange || _changes & RotationChange)
		{
			_transformLock.Lock();
			
			btTransform transform;
			
			getWorldTransform(transform);
			_rigidbody->setWorldTransform(transform);
			
			_transformLock.Unlock();
			
			_changes &= ~(PositionChange | RotationChange);
		}
	}
	
	
	void RigidBodyEntity::SetMass(float mass)
	{
		_mass = mass;
		_changes |= MassChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
	}
	
	void RigidBodyEntity::SetSize(const Vector3& size)
	{
		_size = size;
		_changes |= SizeChange;
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
	}
	
	void RigidBodyEntity::SetPosition(const Vector3& pos)
	{
		Transform::SetPosition(pos);
		_changes |= PositionChange;
		
		_transformLock.Lock();
		_cachedTransform.SetPosition(pos);
		_transformLock.Unlock();
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
	}
	
	void RigidBodyEntity::SetRotation(const Quaternion& rot)
	{
		Transform::SetRotation(rot);
		_changes |= RotationChange;
		
		_transformLock.Lock();
		_cachedTransform.SetRotation(rot);
		_transformLock.Unlock();
		
		World::SharedInstance()->Physics()->ChangedRigidBody(this);
	}
	
	
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
		
		_transformLock.Lock();
		
		_cachedTransform.SetRotation(Quaternion(rot.x(), rot.y(), rot.z(), rot.w()));
		_cachedTransform.SetPosition(Vector3(pos.x(), pos.y(), pos.z()));
		
		_transformLock.Unlock();
	}
	
	btCollisionShape *RigidBodyEntity::GenerateMeshShape()
	{
		_triangleMesh = new btTriangleMesh();
//		_triangleMesh->addTriangle(btVector3(vert1.x, vert1.y, vert1.z), btVector3(vert2.x, vert2.y, vert2.z), btVector3(vert3.x, vert3.y, vert3.z));
		
		return new btBvhTriangleMeshShape(_triangleMesh, true);
	}
}
