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

namespace RN
{
	RigidBodyEntity::RigidBodyEntity()
	{
		_mass = 1.0;
		_size.x = 1.0;
		_size.y = 1.0;
		_size.z = 1.0;
	}
	
	RigidBodyEntity::~RigidBodyEntity()
	{
		
	}
	
	void RigidBodyEntity::Update(float delta)
	{
		
	}
	
	void RigidBodyEntity::PostUpdate()
	{
		SetRotation(_tempRotation);
		SetPosition(_tempPosition);
	}
	
	void RigidBodyEntity::InitializeRigidBody(btDynamicsWorld *world)
	{
		_shape = new btBoxShape(btVector3(_size.x, _size.y, _size.z));
		//shape = new btSphereShape(sz.x);
		btVector3 inertia(0, 0, 0);
		_shape->calculateLocalInertia(_mass, inertia);
		btRigidBody::btRigidBodyConstructionInfo bodyci(_mass, this, _shape, inertia);
		_rigidbody = new btRigidBody(bodyci);
		world->addRigidBody(_rigidbody);
		btTransform trans;
		getWorldTransform(trans);
		setWorldTransform(trans);
	}
	
	void RigidBodyEntity::getWorldTransform(btTransform &worldTrans) const
	{
		const Quaternion& rot = Rotation();
		worldTrans.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
		const Vector3& pos = Position();
		worldTrans.setOrigin(btVector3(pos.x, pos.y, pos.z));
	}
	
	void RigidBodyEntity::setWorldTransform(const btTransform &worldTrans)
	{
		btQuaternion rot = worldTrans.getRotation();
		_tempRotation.x = rot.x();
		_tempRotation.y = rot.y();
		_tempRotation.z = rot.z();
		_tempRotation.w = rot.w();//= Quaternion(rot.x(), rot.y(), rot.z(), rot.w());
		btVector3 pos = worldTrans.getOrigin();
		_tempPosition = Vector3(pos.x(), pos.y(), pos.z());
	}
}
