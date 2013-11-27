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
#include "RNSTL.h"

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
		_cleanUpSignal.Emit(this);
		
		if(_world)
			_world->RemoveSceneNode(this);
		
		LockChildren();
		
		size_t count = _children.GetCount();
		
		for(size_t i = 0; i < count; i ++)
		{
			SceneNode *child = static_cast<SceneNode *>(_children[i]);
			child->_parent = nullptr;
			child->DidUpdate(ChangedParent);
		}
		
		UnlockChildren();
	}
	
	bool SceneNode::Compare(const SceneNode *other) const
	{
		if(other == _parent)
			return false;
		
		return (this < other);
	}
	
	
	void AddDependency(SceneNode *dependency)
	{
	}
	void RemoveDependency(SceneNode *dependency)
	{
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
		stl::lockable_shim<SpinLock> lock1(_parentChildLock);
		stl::lockable_shim<SpinLock> lock2(child->_parentChildLock);
		
		std::lock(lock1, lock2);
		std::unique_lock<stl::lockable_shim<SpinLock>> ulock1(lock1, std::adopt_lock);
		std::unique_lock<stl::lockable_shim<SpinLock>> ulock2(lock2, std::adopt_lock);
		
		if(child->_parent)
			return;
		
		_children.AddObject(child);
		child->_parent = this;
		
		
		ulock1.unlock();
		ulock2.unlock();
		
		DidAddChild(child);
		child->DidUpdate(ChangedParent);
	}
	
	void SceneNode::DetachChild(SceneNode *child)
	{
		stl::lockable_shim<SpinLock> lock1(_parentChildLock);
		stl::lockable_shim<SpinLock> lock2(child->_parentChildLock);
		
		std::lock(lock1, lock2);
		std::unique_lock<stl::lockable_shim<SpinLock>> ulock1(lock1, std::adopt_lock);
		std::unique_lock<stl::lockable_shim<SpinLock>> ulock2(lock2, std::adopt_lock);
		
		if(child->_parent == this)
		{
			child->Retain()->Autorelease();
			child->_parent = nullptr;
			
			_children.RemoveObject(child);
			
			
			ulock1.unlock();
			ulock2.unlock();
			
			child->DidUpdate(ChangedParent);
			DidRemoveChild(child);
		}
	}
	
	void SceneNode::DetachFromParent()
	{
		SceneNode *parent = GetParent();
		if(parent)
			parent->DetachChild(this);
	}

	void SceneNode::LockChildren() const
	{
		_parentChildLock.Lock();
	}
	
	void SceneNode::UnlockChildren() const
	{
		_parentChildLock.Unlock();
	}
	
	SceneNode *SceneNode::GetParent() const
	{
		_parentChildLock.Lock();
		SceneNode *node = _parent;
		_parentChildLock.Unlock();
		
		return node;
	}
	
	
	
	void SceneNode::DidUpdate(uint32 changeSet)
	{
		if(changeSet & ChangedPosition)
			_updated = true;
		
		if(_world)
			_world->SceneNodeDidUpdate(this, changeSet);
		
		if(_parent)
			_parent->ChildDidUpdate(this, changeSet);
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
