//
//  RNSceneNode.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneNode.h"
#include "RNWorld.h"
#include "RNHit.h"

namespace RN
{
	RNDeclareMeta(SceneNode)
	
	SceneNode::SceneNode() :
		_scale(Vector3(1.0f))
	{
		Initialize();
	}
	
	SceneNode::SceneNode(const Vector3& position) :
		_position(position),
		_scale(Vector3(1.0f))
	{
		Initialize();
	}
	
	SceneNode::SceneNode(const Vector3& position, const Quaternion& rotation) :
		_position(position),
		_scale(Vector3(1.0f)),
		_rotation(rotation),
		_euler(rotation.GetEulerAngle())
	{
		Initialize();
	}
	
	SceneNode::~SceneNode()
	{}
	
	void SceneNode::Initialize()
	{
		_parent = nullptr;
		_world  = nullptr;
		_lastFrame = 0;
		
		_priority = Priority::UpdateDontCare;
		renderGroup = 0;
		collisionGroup = 0;
		
		SetBoundingBox(AABB(Vector3(-1.0f), Vector3(1.0f)));
		DidUpdate();
		
		if(World::GetSharedInstance())
			World::GetSharedInstance()->AddSceneNode(this);
	}
	
	void SceneNode::CleanUp()
	{
		if(_world)
			_world->RemoveSceneNode(this);
		
		DetachAllChilds();
	}
	
	
	
	bool SceneNode::IsVisibleInCamera(Camera *camera)
	{
		return true;//camera->InFrustum(_transformedBoundingSphere);
	}
	
	void SceneNode::Render(Renderer *renderer, Camera *camera)
	{
		if(_world)
			_world->SceneNodeWillRender(this);
	}
	
	
	void SceneNode::SetBoundingBox(const AABB& boundingBox, bool calculateBoundingSphere)
	{
		_boundingBox = boundingBox;
		if(calculateBoundingSphere)
			_boundingSphere = Sphere(_boundingBox);
	}
	
	void SceneNode::SetBoundingSphere(const Sphere& boundingSphere)
	{
		_boundingSphere = boundingSphere;
	}
	
	void SceneNode::SetUpdatePriority(Priority priority)
	{
		_priority = priority;
	}
	
	void SceneNode::SetDebugName(const std::string& name)
	{
		_debugName = name;
	}
	
	
	void SceneNode::AttachChild(SceneNode *child)
	{
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
			WillRemoveChild(child);
			_childs.RemoveObject(child);
			
			child->_parent = nullptr;
			child->DidUpdate();
		}
	}
	
	void SceneNode::DetachAllChilds()
	{
		size_t count = _childs.GetCount();
		
		for(size_t i=0; i<count; i++)
		{
			SceneNode *child = _childs.GetObjectAtIndex<SceneNode>(i);
			WillRemoveChild(child);
			
			child->_parent = nullptr;
			child->DidUpdate();
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
	
	Hit SceneNode::CastRay(const Vector3 &position, const Vector3 &direction)
	{
		Hit hit;
		hit = GetBoundingSphere().CastRay(position, direction);
		if(hit.distance > 0.0f)
		{
			hit.node = this;
		}
		
		return hit;
	}
}
