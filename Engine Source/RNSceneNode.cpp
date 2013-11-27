//
//  RNSceneNode.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneNode.h"
#include "RNRenderer.h"
#include "RNWorld.h"
#include "RNHit.h"

namespace RN
{
	RNDeclareMeta(SceneNode)
	
	SceneNode::SceneNode() :
		_position("position", std::bind(&SceneNode::GetPosition, this), std::bind(&SceneNode::SetPosition, this, std::placeholders::_1)),
		_rotation("rotation", std::bind(&SceneNode::GetRotation, this), std::bind(&SceneNode::SetRotation, this, std::placeholders::_1)),
		_scale("scale", Vector3(1.0), std::bind(&SceneNode::GetScale, this), std::bind(&SceneNode::SetScale, this, std::placeholders::_1))
	{
		Initialize();
		
		AddObservable(&_position);
		AddObservable(&_rotation);
		AddObservable(&_scale);
	}
	
	SceneNode::SceneNode(const Vector3& position) :
		SceneNode()
	{
		SetPosition(position);
	}
	
	SceneNode::SceneNode(const Vector3& position, const Quaternion& rotation) :
		SceneNode()
	{
		SetRotation(rotation);
	}
	
	SceneNode::~SceneNode()
	{}
	
	
	void SceneNode::Initialize()
	{
		_parent  = nullptr;
		_world   = nullptr;
		_updated = true;
		_lastFrame = 0;
		_flags     = 0;
		
		_priority      = Priority::UpdateDontCare;
		renderGroup    = 0;
		collisionGroup = 0;
		
		SetBoundingBox(AABB(Vector3(-1.0f), Vector3(1.0f)));
		
		World *world = World::GetSharedInstance();
		
		if(world)
			world->AddSceneNode(this);
	}
	
	void SceneNode::CleanUp()
	{
		if(_world)
			_world->RemoveSceneNode(this);
		
		DetachAllChildren();
	}
	
	
	
	bool SceneNode::IsVisibleInCamera(Camera *camera)
	{
		return camera->InFrustum(_transformedBoundingSphere);
	}
	
	void SceneNode::Render(Renderer *renderer, Camera *camera)
	{
		if(_world)
			_world->SceneNodeWillRender(this);
	}
	
	
	
	void SceneNode::SetFlags(Flags flags)
	{
		_flags = flags;
		DidUpdate(ChangedFlags);
	}
	
	void SceneNode::SetBoundingBox(const AABB& boundingBox, bool calculateBoundingSphere)
	{
		_boundingBox = boundingBox;
		
		if(calculateBoundingSphere)
			_boundingSphere = Sphere(_boundingBox);
		
		_updated = true;
	}
	
	void SceneNode::SetBoundingSphere(const Sphere& boundingSphere)
	{
		_boundingSphere = boundingSphere;
		_updated = true;
	}
	
	void SceneNode::SetPriority(Priority priority)
	{
		_priority = priority;
		DidUpdate(ChangedPriority);
	}
	
	void SceneNode::SetDebugName(const std::string& name)
	{
		_debugName = name;
	}
	
	
	void SceneNode::LookAt(SceneNode *other)
	{
		const RN::Vector3& worldPos = GetWorldPosition();
		const RN::Vector3& point = other->GetWorldPosition();
		
		RN::Quaternion rotation;
		rotation.LookAt(worldPos - point);
		
		SetWorldRotation(rotation);
	}
	
	void SceneNode::AttachChild(SceneNode *child)
	{
		child->DetachFromParent();
		
		_childs.AddObject(child);
		child->_parent = this;
		child->DidUpdate(ChangedParent);
		
		DidAddChild(child);
	}
	
	void SceneNode::DetachChild(SceneNode *child)
	{
		if(child->_parent == this)
		{
			WillRemoveChild(child);
			_childs.RemoveObject(child);
			
			child->_parent = nullptr;
			child->DidUpdate(ChangedParent);
		}
	}
	
	void SceneNode::DetachAllChildren()
	{
		size_t count = _childs.GetCount();
		
		for(size_t i = 0; i < count; i ++)
		{
			SceneNode *child = _childs.GetObjectAtIndex<SceneNode>(i);
			WillRemoveChild(child);
			
			child->_parent = nullptr;
			child->DidUpdate(ChangedParent);
		}
		
		_childs.RemoveAllObjects();
	}
	
	void SceneNode::DetachFromParent()
	{
		if(_parent)
			_parent->DetachChild(this);
	}
	
	void SceneNode::DidUpdate(uint32 changeSet)
	{
		if(changeSet & ChangedPosition)
			_updated = true;
		
		if(_world)
			_world->SceneNodeDidUpdate(this, changeSet);
		
		if(_parent)
			_parent->ChildDidUpdate(this);
	}
	
	void SceneNode::SetAction(const std::function<void (SceneNode *, float)>& action)
	{
		_action = action;
	}
	
	void SceneNode::FillRenderingObject(RenderingObject& object) const
	{
		if(_flags & FlagDrawLate)
			object.flags |= RenderingObject::DrawLate;
	}
	
	
	Hit SceneNode::CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode)
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
