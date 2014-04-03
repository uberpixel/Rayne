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
#include "RNSceneNodeAttachment.h"
#include "RNMessage.h"

namespace RN
{
	RNDefineMeta(SceneNode, Object)
	
	std::atomic<uint64> __SceneNodeIDs;
	
	SceneNode::SceneNode() :
		_position("position", &SceneNode::GetPosition, &SceneNode::SetPosition),
		_rotation("rotation", &SceneNode::GetRotation, &SceneNode::SetRotation),
		_scale("scale", Vector3(1.0), &SceneNode::GetScale, &SceneNode::SetScale),
		_tag("tag", 0, &SceneNode::GetTag, &SceneNode::SetTag),
		_uid(__SceneNodeIDs.fetch_add(1)),
		_lid(-1)
	{
		Initialize();
		AddObservables({ &_tag, &_position, &_rotation, &_scale });
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
	
	SceneNode::SceneNode(const SceneNode *other) :
		SceneNode()
	{
		SceneNode *temp = const_cast<SceneNode *>(other);
		LockGuard<Object *> lock(temp);
		
		SetPosition(other->GetPosition());
		SetRotation(other->GetRotation());
		SetScale(other->GetScale());
		
		renderGroup    = other->renderGroup;
		collisionGroup = other->collisionGroup;
		
		_priority = other->_priority;
		_flags    = other->_flags.load();
		_tag      = other->_tag;
		
		_action = other->_action;
		
		_boundingBox    = other->_boundingBox;
		_boundingSphere = other->_boundingSphere;
		
		other->GetChildren()->Enumerate<RN::SceneNode>([&](RN::SceneNode *node, size_t index, bool &stop){
			AddChild(node->Copy());
		});
	}
	
	SceneNode::~SceneNode()
	{}
	
	
	
	SceneNode::SceneNode(Deserializer *deserializer) :
		SceneNode()
	{
		SetPosition(deserializer->DecodeVector3());
		SetScale(deserializer->DecodeVector3());
		SetRotation(deserializer->DecodeQuaternion());
		
		_boundingBox.minExtend = deserializer->DecodeVector3();
		_boundingBox.maxExtend = deserializer->DecodeVector3();
		_boundingBox.position  = deserializer->DecodeVector3();
		
		uint32 groups = static_cast<uint32>(deserializer->DecodeInt32());
		
		renderGroup    = (groups & 0xff);
		collisionGroup = (groups >> 8);
		
		_priority = static_cast<Priority>(deserializer->DecodeInt32());
		_flags    = static_cast<Flags>(deserializer->DecodeInt32());
		
		_tag = static_cast<Tag>(deserializer->DecodeInt64());
		_lid = deserializer->DecodeInt64();
		
		size_t count = static_cast<size_t>(deserializer->DecodeInt64());
		for(size_t i = 0; i < count; i ++)
		{
			SceneNode *child = static_cast<SceneNode *>(deserializer->DecodeObject());
			
			if(child)
				AddChild(child->Autorelease());
		}
	}
	
	
	void SceneNode::Serialize(Serializer *serializer)
	{
		UpdateInternalData();
		
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(_transformLock);
		
		std::lock(lock1, lock2);
		
		serializer->EncodeVector3(_position);
		serializer->EncodeVector3(_scale);
		serializer->EncodeQuarternion(_rotation);
		
		serializer->EncodeVector3(_boundingBox.minExtend);
		serializer->EncodeVector3(_boundingBox.maxExtend);
		serializer->EncodeVector3(_boundingBox.position);
		
		serializer->EncodeInt32(renderGroup | (collisionGroup << 8));
		serializer->EncodeInt32(static_cast<int32>(_priority));
		serializer->EncodeInt32(_flags);
		serializer->EncodeInt64(_tag);
		serializer->EncodeInt64(_lid);
		
		serializer->EncodeInt64(static_cast<uint64>(_children.GetCount()));
		
		_children.Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {
		
			bool noSave = (node->_flags & Flags::NoSave);
			
			if(noSave)
				serializer->EncodeConditionalObject(node);
			else
				serializer->EncodeObject(node);
			
		});
		
		lock1.unlock();
		lock2.unlock();
	}
	
	
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
			_world->__RemoveSceneNode(this);
		
		_parentChildLock.Lock();
		
		size_t count = _children.GetCount();
		
		for(size_t i = 0; i < count; i ++)
		{
			SceneNode *child = static_cast<SceneNode *>(_children[i]);
			
			child->WillUpdate(ChangeSet::Parent);
			child->_parent = nullptr;
			child->DidUpdate(ChangeSet::Parent);
		}
		
		_children.RemoveAllObjects();
		_parentChildLock.Unlock();
		
		LockGuard<RecursiveSpinLock> lock(_dependenciesLock);
		for(auto& dependecy : _dependencyMap)
		{
			dependecy.second->Disconnect();
		}
		
		_dependencies.clear();
		_dependencyMap.clear();
		
		Unlock();
		
		_attachments.Enumerate<SceneNodeAttachment>([](SceneNodeAttachment *attachment, size_t index, bool &stop) {
			attachment->_node = nullptr;
		});
	}
	
	void SceneNode::RemoveFromWorld()
	{
		if(_world)
			_world->RemoveSceneNode(this);
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
		
		WillUpdate(ChangeSet::Dependencies);
		
		LockGuard<RecursiveSpinLock> lock(_dependenciesLock);
		
		if(_dependencyMap.find(dependency) == _dependencyMap.end())
		{
			Connection *connection = dependency->_cleanUpSignal.Connect(std::bind(&SceneNode::__BreakDependency, this, std::placeholders::_1));
			_dependencyMap.insert(std::make_pair(dependency, connection));
			_dependencies.push_back(dependency);
		}
		
		lock.Unlock();
		DidUpdate(ChangeSet::Dependencies);
	}
	
	void SceneNode::RemoveDependency(SceneNode *dependency)
	{
		if(!dependency)
			return;
		
		WillUpdate(ChangeSet::Dependencies);
		
		LockGuard<RecursiveSpinLock> lock(_dependenciesLock);
		
		auto iterator = _dependencyMap.find(dependency);
		if(iterator != _dependencyMap.end())
		{
			iterator->second->Disconnect();
			
			_dependencyMap.erase(iterator);
			_dependencies.erase(std::find(_dependencies.begin(), _dependencies.end(), dependency));
		}
		
		lock.Unlock();
		DidUpdate(ChangeSet::Dependencies);
	}
	
	void SceneNode::__BreakDependency(SceneNode *dependency)
	{
		WillUpdate(ChangeSet::Dependencies);
		
		LockGuard<RecursiveSpinLock> lock(_dependenciesLock);
		
		_dependencyMap.erase(dependency);
		_dependencies.erase(std::find(_dependencies.begin(), _dependencies.end(), dependency));
		
		lock.Unlock();
		DidUpdate(ChangeSet::Dependencies);
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
		if(_flags & Flags::Hidden)
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
		WillUpdate(ChangeSet::Flags);
		_flags = flags;
		DidUpdate(ChangeSet::Flags);
	}
	
	void SceneNode::SetTag(Tag tag)
	{
		WillUpdate(ChangeSet::Tag);
		_tag = tag;
		DidUpdate(ChangeSet::Tag);
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
		WillUpdate(ChangeSet::Priority);
		_priority = priority;
		DidUpdate(ChangeSet::Priority);
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
		rotation = Quaternion::WithLookAt(worldPos - target, GetUp(), keepUpAxis);
		
		SetWorldRotation(rotation);
	}
	
	// -------------------
	// MARK: -
	// MARK: Children
	// -------------------
	
	void SceneNode::AddChild(SceneNode *child)
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(child->_parentChildLock);
		
		std::lock(lock1, lock2);
		std::unique_lock<stl::lockable_shim<RecursiveSpinLock>> ulock1(lock1, std::adopt_lock);
		std::unique_lock<stl::lockable_shim<RecursiveSpinLock>> ulock2(lock2, std::adopt_lock);
		
		if(child->_parent)
			return;
		
		WillAddChild(child);
		child->WillUpdate(ChangeSet::Parent);
		
		_children.AddObject(child);
		
		child->_parent = this;
		child->DidUpdate(ChangeSet::Parent);
		
		DidAddChild(child);
	}
	
	void SceneNode::RemoveChild(SceneNode *child)
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(child->_parentChildLock);
		
		std::lock(lock1, lock2);
		std::unique_lock<stl::lockable_shim<RecursiveSpinLock>> ulock1(lock1, std::adopt_lock);
		std::unique_lock<stl::lockable_shim<RecursiveSpinLock>> ulock2(lock2, std::adopt_lock);
		
		if(child->_parent == this)
		{
			WillRemoveChild(child);
			child->WillUpdate(ChangeSet::Parent);
			
			child->Retain()->Autorelease();
			child->_parent = nullptr;
			
			_children.RemoveObject(child);
			
			child->DidUpdate(ChangeSet::Parent);
			DidRemoveChild(child);
		}
	}
	
	void SceneNode::RemoveFromParent()
	{
		SceneNode *parent = GetParent();
		if(parent)
			parent->RemoveChild(this);
	}
	
	const Array *SceneNode::GetChildren() const
	{
		LockGuard<decltype(_parentChildLock)> lock(_parentChildLock);
		return _children.Copy()->Autorelease();
	}
	
	bool SceneNode::HasChildren() const
	{
		LockGuard<decltype(_parentChildLock)> lock(_parentChildLock);
		return (_children.GetCount() > 0);
	}
	
	SceneNode *SceneNode::GetParent() const
	{
		_parentChildLock.Lock();
		SceneNode *node = _parent;
		_parentChildLock.Unlock();
		
		return node;
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
			
		WillUpdate(ChangeSet::Attachments);
		
		_attachments.AddObject(attachment);
		
		attachment->_node = this;
		attachment->DidAddToParent();
		
		DidUpdate(ChangeSet::Attachments);
	}
	
	void SceneNode::RemoveAttachment(SceneNodeAttachment *attachment)
	{
		LockGuard<decltype(_attachmentsLock)> lock(_attachmentsLock);
		
		if(attachment->_node != this)
			throw Exception(Exception::Type::InvalidArgumentException, "Attachments must be removed from their parents!");
		
		WillUpdate(ChangeSet::Attachments);
		
		attachment->WillRemoveFromParent();
		attachment->_node = nullptr;
		_attachments.RemoveObject(attachment);
		
		DidUpdate(ChangeSet::Attachments);
	}
	
	SceneNodeAttachment *SceneNode::GetAttachment(MetaClassBase *metaClass) const
	{
		LockGuard<decltype(_attachmentsLock)> lock(_attachmentsLock);
		
		SceneNodeAttachment *match = nullptr;
		
		_attachments.Enumerate<SceneNodeAttachment>([&](SceneNodeAttachment *attachment, size_t index, bool &stop) {
			if(attachment->IsKindOfClass(metaClass))
			{
				match = attachment;
				stop = true;
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
	
	void SceneNode::UpdateAttachments(float delta)
	{
		LockGuard<decltype(_attachmentsLock)> lock(_attachmentsLock);
		
		_attachments.Enumerate<SceneNodeAttachment>([=](SceneNodeAttachment *attachment, size_t index, bool &stop) {
			attachment->Update(delta);
		});
	}
	
	void SceneNode::UpdateAttachmentsEditMode(float delta)
	{
		LockGuard<decltype(_attachmentsLock)> lock(_attachmentsLock);
		
		_attachments.Enumerate<SceneNodeAttachment>([=](SceneNodeAttachment *attachment, size_t index, bool &stop) {
			attachment->UpdateEditMode(delta);
		});
	}
	
	// -------------------
	// MARK: -
	// MARK: Updates
	// ------------------
	
	void SceneNode::WillUpdate(ChangeSet changeSet)
	{
		if(_parent)
			_parent->ChildWillUpdate(this, changeSet);
		
		LockGuard<decltype(_attachmentsLock)> lock(_attachmentsLock);
		_attachments.Enumerate<SceneNodeAttachment>([&](SceneNodeAttachment *attachment, size_t index, bool &stop) {
			attachment->__WillUpdate(changeSet);
		});
	}
	
	void SceneNode::DidUpdate(ChangeSet changeSet)
	{
		if(changeSet & ChangeSet::Position)
			_updated = true;
		
		if(changeSet & ChangeSet::Parent && _parent == nullptr)
		{
			LockGuard<decltype(_transformLock)> lock(_transformLock);
			
			if(_parent == nullptr)
			{
				_position = _worldPosition;
				_rotation = _worldRotation;
				_euler    = _worldEuler;
				_scale    = _worldScale;
				
				_updated = true;
			}
			else
			{
				Vector3 position(_position);
				Quaternion rotation(_rotation);
				Vector3 scale(_scale);
				
				SetWorldPosition(position);
				SetWorldRotation(rotation);
				SetWorldScale(scale);
			}
		}
		
		if(changeSet & ChangeSet::World)
		{
			LockGuard<decltype(_parentChildLock)> lock(_parentChildLock);
			
			_children.Enumerate<RN::SceneNode>([&](RN::SceneNode *node, size_t index, bool &stop) {
				node->_world = _world;
				node->DidUpdate(ChangeSet::World);
			});
			
			MessageCenter::GetSharedInstance()->PostMessage(kRNSceneNodeDidUpdateWorldMessage, this, nullptr);
		}
		
		if(_world)
			_world->SceneNodeDidUpdate(this, changeSet);
		
		if(_parent)
			_parent->ChildDidUpdate(this, changeSet);
		
		LockGuard<decltype(_attachmentsLock)> lock(_attachmentsLock);
		_attachments.Enumerate<SceneNodeAttachment>([&](SceneNodeAttachment *attachment, size_t index, bool &stop) {
			attachment->__DidUpdate(changeSet);
		});
	}
	
	void SceneNode::FillRenderingObject(RenderingObject& object) const
	{
		if(_flags & Flags::DrawLate)
			object.flags |= RenderingObject::DrawLate;
		
		object.transform = &_worldTransform;
		object.rotation  = &_worldRotation;
		
		_attachments.Enumerate<SceneNodeAttachment>([&](SceneNodeAttachment *attachment, size_t index, bool &stop) {
			attachment->UpdateRenderingObject(object);
		});
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
