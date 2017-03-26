//
//  RNSceneNode.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneNode.h"
#include "RNScene.h"
#include "RNSceneNodeAttachment.h"

namespace RN
{
	RNDefineMeta(SceneNode, Object)

	std::atomic<uint64> __SceneNodeIDs;

	SceneNode::SceneNode() :
		_uid(__SceneNodeIDs.fetch_add(1)),
		_lid(static_cast<uint64>(-1)),
		_scene(nullptr),
		_sceneEntry(this),
		_tag("tag", 0, &SceneNode::GetTag, &SceneNode::SetTag),
		_position("position", &SceneNode::GetPosition, &SceneNode::SetPosition),
		_scale("scale", Vector3(1.0), &SceneNode::GetScale, &SceneNode::SetScale),
		_rotation("rotation", &SceneNode::GetRotation, &SceneNode::SetRotation),
		_attachments(nullptr)
	{
		Initialize();
		AddObservables({ &_tag, &_position, &_rotation, &_scale });
		SetBoundingBox(AABB(Vector3(-1.0f), Vector3(1.0f)));
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

		LockWrapper<Object *> wrapper(temp);
		LockGuard<LockWrapper<Object *>> lock(wrapper);

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

		if(_attachments)
			_attachments->Release();
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

	void SceneNode::SetBoundingBox(const AABB &boundingBox, bool calculateBoundingSphere)
	{
		_boundingBox = boundingBox;

		if(calculateBoundingSphere)
			_boundingSphere = Sphere(_boundingBox);

		_updated = true;
	}

	void SceneNode::SetBoundingSphere(const Sphere &boundingSphere)
	{
		_boundingSphere = boundingSphere;
		_updated = true;
	}

	void SceneNode::LookAt(const RN::Vector3 &target, bool keepUpAxis)
	{
		const RN::Vector3 &worldPos = GetWorldPosition();

		RN::Quaternion rotation;
		rotation = Quaternion::WithLookAt(worldPos - target, GetUp(), keepUpAxis);

		SetWorldRotation(rotation);
	}

	bool SceneNode::HasFlags(Flags flags) const
	{
		return (_flags.load(std::memory_order_acquire) & flags);
	}
	SceneNode::Flags SceneNode::RemoveFlags(Flags flags)
	{
		return _flags.fetch_and(flags, std::memory_order_acq_rel) & flags;
	}
	SceneNode::Flags SceneNode::AddFlags(Flags flags)
	{
		return _flags.fetch_or(flags, std::memory_order_acq_rel) | flags;
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

	void SceneNode::AddAttachment(SceneNodeAttachment *attachment)
	{
		if(!_attachments)
			_attachments = new Array();

		_attachments->AddObject(attachment);
		attachment->_node = this;
	}

	void SceneNode::RemoveAttachment(SceneNodeAttachment *attachment)
	{
		attachment->_node = nullptr;
		_attachments->RemoveObject(attachment);
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
	// MARK: Rendering
	// ------------------

	bool SceneNode::CanRender(Renderer *renderer, Camera *camera) const
	{
		if(_flags.load(std::memory_order_acquire) & Flags::Hidden)
			return false;

		return camera->InFrustum(GetBoundingSphere());
	}

	void SceneNode::Render(Renderer *renderer, Camera *camera) const
	{}

	// -------------------
	// MARK: -
	// MARK: Updates
	// ------------------

	void SceneNode::Update(float delta)
	{
		if(_attachments)
		{
			_attachments->Enumerate<SceneNodeAttachment>([delta](SceneNodeAttachment *attachment, size_t index, bool &stop) {
				attachment->Update(delta);
			});
		}
	}

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

	void SceneNode::UpdateInternalData() const
	{
		if(_updated)
		{
			_localTransform = Matrix::WithTranslation(_position);
			_localTransform.Rotate(_rotation);
			_localTransform.Scale(_scale);

			if(_parent)
			{
				_parent->UpdateInternalData();

				_worldPosition = _parent->_worldPosition + _parent->_worldRotation.GetRotatedVector(_position);
				_worldRotation = _parent->_worldRotation * _rotation;
				_worldScale = _parent->_worldScale * _scale;
				_worldEuler = _parent->_worldEuler + _euler;

				_worldTransform = _parent->_worldTransform * _localTransform;
			}
			else
			{
				_worldPosition = _position;
				_worldRotation = _rotation;
				_worldScale = _scale;
				_worldEuler = _euler;

				_worldTransform = _localTransform;
			}

			_transformedBoundingBox = _boundingBox;
			_transformedBoundingBox.position = _worldPosition;
			_transformedBoundingBox *= _worldScale;
			_transformedBoundingBox.SetRotation(_worldRotation);

			_transformedBoundingSphere = _boundingSphere;
			_transformedBoundingSphere.position = _worldPosition;
			_transformedBoundingSphere *= _worldScale;
			_transformedBoundingSphere.SetRotation(_worldRotation);

			_updated = false;
			_children->Enumerate<SceneNode>([](SceneNode *child, size_t index, bool &stop) {
				child->_updated = true;
			});
		}
	}
}
