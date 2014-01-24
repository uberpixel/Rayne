//
//  RNSceneNode.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
		
		World *world = World::GetActiveWorld();
		
		if(world)
			world->AddSceneNode(this);
	}
	
	void SceneNode::CleanUp()
	{
		Lock();
		
		_cleanUpSignal.Emit(this);
		
		if(_world)
			_world->RemoveSceneNode(this);
		
		LockChildren();
		
		size_t count = _children.GetCount();
		
		for(size_t i = 0; i < count; i ++)
		{
			SceneNode *child = static_cast<SceneNode *>(_children[i]);
			
			child->WillUpdate(ChangedParent);
			child->_parent = nullptr;
			child->DidUpdate(ChangedParent);
		}
		
		_children.RemoveAllObjects();
		UnlockChildren();
		
		LockGuard<RecursiveSpinLock> lock(_dependenciesLock);
		for(auto& dependecy : _dependencyMap)
		{
			dependecy.second->Disconnect();
		}
		
		_dependencies.clear();
		_dependencyMap.clear();
		
		Unlock();
		
		_attachments.Enumerate<SceneNodeAttachment>([](SceneNodeAttachment *attachment, size_t index, bool *stop) {
			attachment->_node = nullptr;
		});
	}
	
	bool SceneNode::Compare(const SceneNode *other) const
	{
		if(other == _parent)
			return false;
		
		for(SceneNode *node : _dependencies)
		{
			if(node == other)
				return false;
		}
		
		return (this < other);
	}
	
	
	// -------------------
	// MARK: -
	// MARK: Dependencies
	// -------------------
	
	void SceneNode::AddDependency(SceneNode *dependency)
	{
		if(!dependency)
			return;
		
		WillUpdate(ChangedDependencies);
		
		LockGuard<RecursiveSpinLock> lock(_dependenciesLock);
		
		if(_dependencyMap.find(dependency) == _dependencyMap.end())
		{
			Connection *connection = dependency->_cleanUpSignal.Connect(std::bind(&SceneNode::__BreakDependency, this, std::placeholders::_1));
			_dependencyMap.insert(std::make_pair(dependency, connection));
			_dependencies.push_back(dependency);
		}
		
		lock.Unlock();
		DidUpdate(ChangedDependencies);
	}
	
	void SceneNode::RemoveDependency(SceneNode *dependency)
	{
		if(!dependency)
			return;
		
		WillUpdate(ChangedDependencies);
		
		LockGuard<RecursiveSpinLock> lock(_dependenciesLock);
		
		auto iterator = _dependencyMap.find(dependency);
		if(iterator != _dependencyMap.end())
		{
			iterator->second->Disconnect();
			
			_dependencyMap.erase(iterator);
			_dependencies.erase(std::find(_dependencies.begin(), _dependencies.end(), dependency));
		}
		
		lock.Unlock();
		DidUpdate(ChangedDependencies);
	}
	
	void SceneNode::__BreakDependency(SceneNode *dependency)
	{
		WillUpdate(ChangedDependencies);
		
		LockGuard<RecursiveSpinLock> lock(_dependenciesLock);
		
		_dependencyMap.erase(dependency);
		_dependencies.erase(std::find(_dependencies.begin(), _dependencies.end(), dependency));
		
		lock.Unlock();
		DidUpdate(ChangedDependencies);
	}
	
	
	bool SceneNode::CanUpdate(FrameID frame)
	{
		LockGuard<RecursiveSpinLock> lock1(_parentChildLock);
		
		if(_parent)
			return (_parent->_lastFrame.load() >= frame);
		
		lock1.Unlock();
		
		LockGuard<RecursiveSpinLock> lock2(_dependenciesLock);
		
		for(SceneNode *node : _dependencies)
		{
			if(node->_lastFrame.load() < frame)
				return false;
		}
		
		return true;
	}
	
	bool SceneNode::IsVisibleInCamera(Camera *camera)
	{
		if(_flags & FlagHidden)
			return false;
		
		return camera->InFrustum(GetBoundingSphere());
	}
	
	void SceneNode::Render(Renderer *renderer, Camera *camera)
	{
		if(_world)
			_world->SceneNodeWillRender(this);
	}
	
	
	// -------------------
	// MARK: -
	// MARK: Setter
	// -------------------
	
	void SceneNode::SetFlags(Flags flags)
	{
		WillUpdate(ChangedFlags);
		_flags = flags;
		DidUpdate(ChangedFlags);
	}
	
	void SceneNode::SetRenderGroup(uint8 group)
	{
		renderGroup = group;
	}
	
	void SceneNode::SetCollisionGroup(uint8 group)
	{
		collisionGroup = group;
	}
	
	void SceneNode::SetBoundingBox(const AABB& boundingBox, bool calculateBoundingSphere)
	{
		_transformLock.Lock();
		_boundingBox = boundingBox;
		
		if(calculateBoundingSphere)
			_boundingSphere = Sphere(_boundingBox);
		
		_updated = true;
		_transformLock.Unlock();
	}
	
	void SceneNode::SetBoundingSphere(const Sphere& boundingSphere)
	{
		_transformLock.Lock();
		_boundingSphere = boundingSphere;
		_updated = true;
		_transformLock.Unlock();
	}
	
	void SceneNode::SetPriority(Priority priority)
	{
		WillUpdate(ChangedPriority);
		_priority = priority;
		DidUpdate(ChangedPriority);
	}
	
	void SceneNode::SetDebugName(const std::string& name)
	{
		_debugName = name;
	}
	
	void SceneNode::SetAction(const std::function<void (SceneNode *, float)>& action)
	{
		_action = action;
	}
	
	void SceneNode::LookAt(const RN::Vector3 &target, bool keepUpAxis)
	{
		const RN::Vector3& worldPos = GetWorldPosition();
		
		RN::Quaternion rotation;
		rotation.LookAt(worldPos - target, GetUp(), keepUpAxis);
		
		SetWorldRotation(rotation);
	}
	
	// -------------------
	// MARK: -
	// MARK: Children
	// -------------------
	
	void SceneNode::AttachChild(SceneNode *child)
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(child->_parentChildLock);
		
		std::lock(lock1, lock2);
		std::unique_lock<stl::lockable_shim<RecursiveSpinLock>> ulock1(lock1, std::adopt_lock);
		std::unique_lock<stl::lockable_shim<RecursiveSpinLock>> ulock2(lock2, std::adopt_lock);
		
		if(child->_parent)
			return;
		
		WillAddChild(child);
		child->WillUpdate(ChangedParent);
		
		_children.AddObject(child);
		
		child->_parent = this;
		child->DidUpdate(ChangedParent);
		
		DidAddChild(child);
	}
	
	void SceneNode::DetachChild(SceneNode *child)
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(child->_parentChildLock);
		
		std::lock(lock1, lock2);
		std::unique_lock<stl::lockable_shim<RecursiveSpinLock>> ulock1(lock1, std::adopt_lock);
		std::unique_lock<stl::lockable_shim<RecursiveSpinLock>> ulock2(lock2, std::adopt_lock);
		
		if(child->_parent == this)
		{
			WillRemoveChild(child);
			child->WillUpdate(ChangedParent);
			
			child->Retain()->Autorelease();
			child->_parent = nullptr;
			
			_children.RemoveObject(child);
			
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
	
	SceneNode *SceneNode::GetParent() const
	{
		_parentChildLock.Lock();
		SceneNode *node = _parent;
		_parentChildLock.Unlock();
		
		return node;
	}

	void SceneNode::LockChildren() const
	{
		_parentChildLock.Lock();
	}
	
	void SceneNode::UnlockChildren() const
	{
		_parentChildLock.Unlock();
	}
	
	// -------------------
	// MARK: -
	// MARK: Attachments
	// -------------------
	
	void SceneNode::AddAttachment(SceneNodeAttachment *attachment)
	{
		LockGuard<decltype(_attachmentsLock)> lock(_attachmentsLock);
		
		if(attachment->_node)
			throw Exception(Exception::Type::InvalidArgumentException, "Attachments mustn't have a parent!");
			
		WillUpdate(ChangedAttachments);
		
		_attachments.AddObject(attachment);
		
		attachment->_node = this;
		attachment->DidAddToParent();
		
		DidUpdate(ChangedAttachments);
	}
	
	void SceneNode::RemoveAttachment(SceneNodeAttachment *attachment)
	{
		LockGuard<decltype(_attachmentsLock)> lock(_attachmentsLock);
		
		if(attachment->_node != this)
			throw Exception(Exception::Type::InvalidArgumentException, "Attachments must be removed from their parents!");
		
		WillUpdate(ChangedAttachments);
		
		attachment->WillRemoveFromParent();
		attachment->_node = nullptr;
		_attachments.RemoveObject(attachment);
		
		DidUpdate(ChangedAttachments);
	}
	
	SceneNodeAttachment *SceneNode::GetAttachment(MetaClassBase *metaClass) const
	{
		LockGuard<decltype(_attachmentsLock)> lock(_attachmentsLock);
		
		SceneNodeAttachment *match = nullptr;
		
		_attachments.Enumerate<SceneNodeAttachment>([&](SceneNodeAttachment *attachment, size_t index, bool *stop) {
			if(attachment->IsKindOfClass(metaClass))
			{
				match = attachment;
				*stop = true;
			}
		});
		
		return match;
	}
	
	Array *SceneNode::GetAttachments() const
	{
		LockGuard<decltype(_attachmentsLock)> lock(_attachmentsLock);
		Array *attachments = new Array(&_attachments);
		
		return attachments->Autorelease();
	}
	
	// -------------------
	// MARK: -
	// MARK: Updates
	// ------------------
	
	void SceneNode::WillUpdate(uint32 changeSet)
	{
		if(_parent)
			_parent->ChildWillUpdate(this, changeSet);
		
		LockGuard<decltype(_attachmentsLock)> lock(_attachmentsLock);
		_attachments.Enumerate<SceneNodeAttachment>([&](SceneNodeAttachment *attachment, size_t index, bool *stop) {
			attachment->__WillUpdate(changeSet);
		});
	}
	
	void SceneNode::DidUpdate(uint32 changeSet)
	{
		if(changeSet & ChangedPosition)
			_updated = true;
		
		if(_world)
			_world->SceneNodeDidUpdate(this, changeSet);
		
		if(_parent)
			_parent->ChildDidUpdate(this, changeSet);
		
		LockGuard<decltype(_attachmentsLock)> lock(_attachmentsLock);
		_attachments.Enumerate<SceneNodeAttachment>([&](SceneNodeAttachment *attachment, size_t index, bool *stop) {
			attachment->__DidUpdate(changeSet);
		});
	}
	
	void SceneNode::FillRenderingObject(RenderingObject& object) const
	{
		if(_flags & FlagDrawLate)
			object.flags |= RenderingObject::DrawLate;
		
		object.transform = &_worldTransform;
		object.rotation  = &_worldRotation;
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
