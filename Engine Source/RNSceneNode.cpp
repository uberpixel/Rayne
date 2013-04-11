//
//  RNSceneNode.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneNode.h"
#include "RNWorld.h"

namespace RN
{
	RNDeclareMeta(SceneNode)
	
	SceneNode::SceneNode() :
		_scale(Vector3(1.0f))
	{
		_parent = 0;
		_world  = 0;
		_lastFrame = 0;
		
		DidUpdate();
		World::SharedInstance()->AddSceneNode(this);
	}
	
	SceneNode::SceneNode(const Vector3& position) :
		_position(position),
		_scale(Vector3(1.0f))
	{
		_parent = 0;
		_world  = 0;
		_lastFrame = 0;
		
		DidUpdate();
		World::SharedInstance()->AddSceneNode(this);
	}
	
	SceneNode::SceneNode(const Vector3& position, const Quaternion& rotation) :
		_position(position),
		_scale(Vector3(1.0f)),
		_rotation(rotation),
		_euler(rotation.EulerAngle())
	{
		_parent = 0;
		_world  = 0;
		_lastFrame = 0;
		
		DidUpdate();
		World::SharedInstance()->AddSceneNode(this);
	}
	
	SceneNode::~SceneNode()
	{
		DetachAllChilds();
		
		if(_world)
			_world->RemoveSceneNode(this);
	}
	
	
	bool SceneNode::IsVisibleInCamera(Camera *camera)
	{
		return camera->InFrustum(WorldPosition(), _boundingSphere);
	}
	
	
	void SceneNode::AttachChild(SceneNode *child)
	{
		child->Retain();
		child->DetachFromParent();
		
		_childs.AddObject(child);
		child->_parent = this;
		child->DidUpdate();
		
		DidAddChild(child);
	}
	
	void SceneNode::DetachChild(SceneNode *child)
	{
		if(child->_parent == this)
		{
			_childs.RemoveObject(child);
			
			child->_parent = 0;
			child->DidUpdate();
			child->Release();
		}
	}
	
	void SceneNode::DetachAllChilds()
	{
		machine_uint count = _childs.Count();
		
		for(machine_uint i=0; i<count; i++)
		{
			SceneNode *child = _childs.ObjectAtIndex(i);
			WillRemoveChild(child);
			
			child->_parent = 0;
			child->DidUpdate();
			child->Release();
		}
		
		_childs.RemoveAllObjects();
	}
	
	void SceneNode::DetachFromParent()
	{
		if(_parent)
			_parent->DetachChild(this);
	}
	
	void SceneNode::DidUpdate()
	{
		_updated = true;
		
		if(_world)
			_world->SceneNodeUpdated(this);
		
		if(_parent)
			_parent->ChildDidUpdate(this);
	}
	
	void SceneNode::SetAction(const std::function<void (SceneNode *, float)>& action)
	{
		_action = action;
	}
}
