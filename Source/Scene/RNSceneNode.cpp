//
//  RNSceneNode.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneNode.h"
#include "RNScene.h"

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
		_lid(-1),
		_sceneEntry(this),
		_scene(nullptr)
	{
		Initialize();
		AddObservables({ &_tag, &_position, &_rotation, &_scale });
	}

	SceneNode::SceneNode(const Vector3 &position) :
		SceneNode()
	{
		SetPosition(position);
	}

	SceneNode::SceneNode(const Vector3 &position, const Quaternion &rotation) :
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

		_renderGroup    = other->_renderGroup;
		_collisionGroup = other->_collisionGroup;

		_priority = other->_priority;
		_flags    = other->_flags.load();
		_tag      = other->_tag;

		other->GetChildren()->Enumerate<RN::SceneNode>([&](RN::SceneNode *node, size_t index, bool &stop){
			AddChild(node->Copy());
		});
	}

	SceneNode::~SceneNode()
	{
		_children->Release();
	}



	SceneNode::SceneNode(Deserializer *deserializer) :
		SceneNode()
	{
		SetPosition(deserializer->DecodeVector3());
		SetScale(deserializer->DecodeVector3());
		SetRotation(deserializer->DecodeQuaternion());

		uint32 groups = static_cast<uint32>(deserializer->DecodeInt32());

		_renderGroup    = (groups & 0xff);
		_collisionGroup = (groups >> 8);

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


	void SceneNode::Serialize(Serializer *serializer) const
	{
		UpdateInternalData();

		serializer->EncodeVector3(_position);
		serializer->EncodeVector3(_scale);
		serializer->EncodeQuarternion(_rotation);

		serializer->EncodeInt32(_renderGroup | (_collisionGroup << 8));
		serializer->EncodeInt32(static_cast<int32>(_priority));
		serializer->EncodeInt32(_flags);
		serializer->EncodeInt64(_tag);
		serializer->EncodeInt64(_lid);

		serializer->EncodeInt64(static_cast<uint64>(_children->GetCount()));

		_children->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {

			bool noSave = (node->_flags & Flags::NoSave);

			if(noSave)
				serializer->EncodeConditionalObject(node);
			else
				serializer->EncodeObject(node);

		});
	}


	void SceneNode::Initialize()
	{
		_children = new Array();
		_parent = nullptr;
		_scene = nullptr;
		_updated = true;
		_flags = 0;

		_priority = Priority::UpdateNormal;
		_renderGroup = 0;
		_collisionGroup = 0;
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
		_renderGroup = group;
	}

	void SceneNode::SetCollisionGroup(uint8 group)
	{
		_collisionGroup = group;
	}

	void SceneNode::SetPriority(Priority priority)
	{
		WillUpdate(ChangeSet::Priority);
		_priority = priority;
		DidUpdate(ChangeSet::Priority);
	}

	void SceneNode::LookAt(const RN::Vector3 &target, bool keepUpAxis)
	{
		const RN::Vector3 &worldPos = GetWorldPosition();

		RN::Quaternion rotation;
		rotation = Quaternion::WithLookAt(worldPos - target, GetUp(), keepUpAxis);

		SetWorldRotation(rotation);
	}

	// -------------------
	// MARK: -
	// MARK: Scene
	// -------------------

	void SceneNode::UpdateScene(Scene *scene)
	{
		if(_scene)
		{
			_children->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {
				_scene->RemoveNode(node);
			});
		}

		_scene = scene;

		if(_scene)
		{
			_children->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {
				scene->AddNode(node);
			});
		}
	}

	void SceneNode::__CompleteAttachmentWithScene(Scene *scene)
	{
		if(_scene == scene)
			return;

		if(_scene)
			_scene->RemoveNode(this);

		if(scene)
			scene->AddNode(this);
	}

	// -------------------
	// MARK: -
	// MARK: Children
	// -------------------

	void SceneNode::AddChild(SceneNode *child)
	{
		child->RemoveFromParent();

		WillAddChild(child);
		child->WillUpdate(ChangeSet::Parent);

		_children->AddObject(child);

		child->_parent = this;
		child->DidUpdate(ChangeSet::Parent);
		child->__CompleteAttachmentWithScene(_scene);

		DidAddChild(child);
	}

	void SceneNode::RemoveChild(SceneNode *child)
	{
		if(child->_parent == this)
		{
			WillRemoveChild(child);
			child->WillUpdate(ChangeSet::Parent);

			child->Retain()->Autorelease();
			child->_parent = nullptr;

			_children->RemoveObject(child);

			child->DidUpdate(ChangeSet::Parent);
			child->__CompleteAttachmentWithScene(nullptr);

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
		return _children;
	}

	SceneNode *SceneNode::GetParent() const
	{
		return _parent;
	}


	// -------------------
	// MARK: -
	// MARK: Updates
	// ------------------

	void SceneNode::WillUpdate(ChangeSet changeSet)
	{
		if(_parent)
			_parent->ChildWillUpdate(this, changeSet);
	}

	void SceneNode::DidUpdate(ChangeSet changeSet)
	{
		if(changeSet & ChangeSet::Position)
			_updated = true;

		if(changeSet & ChangeSet::Parent && _parent == nullptr)
		{
			SetWorldPosition(_position);
			SetWorldRotation(_rotation);
			SetWorldScale(_scale);
		}

		if(_parent)
			_parent->ChildDidUpdate(this, changeSet);
	}
}
