//
//  RNTransform.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNTransform.h"
#include "RNWorld.h"

namespace RN
{
	RNDeclareMeta(Transform)
	
	Transform::Transform() :
		_scale(Vector3(1.0f))
	{
		_parent = 0;
		DidUpdate();
		
		World::SharedInstance()->AddTransform(this);
	}
	
	Transform::Transform(const Vector3& position) :
		_position(position),
		_scale(Vector3(1.0f))
	{
		_parent = 0;
		DidUpdate();
		
		World::SharedInstance()->AddTransform(this);
	}
	
	Transform::Transform(const Vector3& position, const Quaternion& rotation) :
		_position(position),
		_scale(Vector3(1.0f)),
		_rotation(rotation),
		_euler(rotation.EulerAngle())
	{
		_parent = 0;
		DidUpdate();
		
		World::SharedInstance()->AddTransform(this);
	}
	
	Transform::~Transform()
	{
		DetachAllChilds();
		World::SharedInstance()->RemoveTransform(this);
	}
	
	
	
	void Transform::AttachChild(Transform *child)
	{
		child->Retain();
		child->DetachFromParent();
		
		_childs.AddObject(child);
		child->_parent = this;
		child->DidUpdate();
	}
	
	void Transform::DetachChild(Transform *child)
	{
		if(child->_parent == this)
		{
			_childs.RemoveObject(child);
			child->_parent = 0;
			child->DidUpdate();
			child->Release();
		}
	}
	
	void Transform::DetachAllChilds()
	{
		machine_uint count = _childs.Count();
		
		for(machine_uint i=0; i<count; i++)
		{
			Transform *child = _childs.ObjectAtIndex(i);
			child->_parent = 0;
			child->DidUpdate();
			child->Release();
		}
		
		_childs.RemoveAllObjects();
	}
	
	void Transform::DetachFromParent()
	{
		if(_parent)
			_parent->DetachChild(this);
	}
}
