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
		_sceneUpdateEntry(this),
		_sceneRenderEntry(this),
		_scheduledForRemovalFromScene(false),
		_uid(__SceneNodeIDs.fetch_add(1)),
		_lid(static_cast<uint64>(-1)),
		_sceneInfo(nullptr),
		_tag("tag", 0, &SceneNode::GetTag, &SceneNode::SetTag),
		_position("position", &SceneNode::GetPosition, &SceneNode::SetPosition),
		_scale("scale", Vector3(1.0), &SceneNode::GetScale, &SceneNode::SetScale),
		_rotation("rotation", &SceneNode::GetRotation, &SceneNode::SetRotation),
		_attachments(nullptr)
	{
		Initialize();
		AddObservables({&_tag, &_position, &_rotation, &_scale});
		SetBoundingBox(AABB(Vector3(-1.0f), Vector3(1.0f)));
	}

	SceneNode::SceneNode(const PositionType &position) :
		SceneNode()
	{
		SetPosition(position);
	}

	SceneNode::SceneNode(const PositionType &position, const Quaternion &rotation) :
		SceneNode(position)
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

		_renderGroup = other->_renderGroup;
		_collisionGroup = other->_collisionGroup;
		_maximumRenderDistance = other->_maximumRenderDistance;

		_updatePriority = other->_updatePriority;
		_renderPriority = other->_renderPriority;
		_flags = other->_flags.load();
		_tag = other->_tag;

		other->GetChildren()->Enumerate<RN::SceneNode>([&](RN::SceneNode *node, size_t index, bool &stop) {
			SceneNode *nodeCopy = node->Copy();
			AddChild(nodeCopy);
			nodeCopy->Release();
		});
	}

	SceneNode::~SceneNode()
	{
		//Set parent of child nodes to null when it gets released
		_children->Enumerate<SceneNode>([](SceneNode *child, size_t index, bool &stop) {
			child->WillUpdate(ChangeSet::Parent);
			child->_parent = nullptr;
			child->DidUpdate(ChangeSet::Parent);
			child->__CompleteAttachmentWithScene(nullptr);
		});
		_children->Release();

		if(_attachments)
		{
			_attachments->Enumerate<SceneNodeAttachment>([](SceneNodeAttachment *attachment, size_t index, bool &stop) {
				attachment->_node = nullptr;
			});
			_attachments->Release();
		}
	}


	SceneNode::SceneNode(Deserializer *deserializer) :
		SceneNode()
	{
#if RN_ENABLE_UNIVERSE_SCALE
		SetPosition(deserializer->DecodeDVector3());
#else
		SetPosition(deserializer->DecodeVector3());
#endif
		SetScale(deserializer->DecodeVector3());
		SetRotation(deserializer->DecodeQuaternion());

		uint32 groups = static_cast<uint32>(deserializer->DecodeInt32());

		_renderGroup = (groups & 0xffff);
		_collisionGroup = (groups >> 16);

		_updatePriority = static_cast<UpdatePriority>(deserializer->DecodeInt32());
		_renderPriority = deserializer->DecodeInt32();
		_flags = static_cast<Flags>(deserializer->DecodeInt32());

		_tag = static_cast<Tag>(deserializer->DecodeInt64());
		_lid = deserializer->DecodeInt64();

		size_t count = static_cast<size_t>(deserializer->DecodeInt64());
		for(size_t i = 0; i < count; i++)
		{
			SceneNode *child = static_cast<SceneNode *>(deserializer->DecodeObject());

			if(child)
				AddChild(child->Autorelease());
		}
	}


	void SceneNode::Serialize(Serializer *serializer) const
	{
		UpdateInternalData();

#if RN_ENABLE_UNIVERSE_SCALE
		serializer->EncodeDVector3(_position);
#else
		serializer->EncodeVector3(_position);
#endif
		serializer->EncodeVector3(_scale);
		serializer->EncodeQuarternion(_rotation);

		serializer->EncodeInt32(_renderGroup | (_collisionGroup << 16));
		serializer->EncodeInt32(static_cast<int32>(_updatePriority));
		serializer->EncodeInt32(_renderPriority);
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
		_sceneInfo = nullptr;
		_transformVersion = 1;
#if RN_ENABLE_UNIVERSE_SCALE
		_linearTransformVersion = 1;
#endif
		_boundsDirty = true;
		_cachedWorldDataVersion = 0;
		_cachedMatrixVersion = 0;
		_cachedInverseMatrixVersion = 0;
		_cachedBoundsVersion = 0;
		_flags = 0;

		_updatePriority = UpdatePriority::UpdateNormal;
		_renderPriority = RenderPriority::RenderNormal;
		_renderGroup = 1;
		_collisionGroup = 0;
		_needsRenderPreparation = false;
		_maximumRenderDistance = 0.0f;
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

	void SceneNode::SetRenderGroup(uint16 group)
	{
		_renderGroup = group;
	}

	void SceneNode::SetCollisionGroup(uint8 group)
	{
		_collisionGroup = group;
	}

	void SceneNode::SetMaximumRenderDistance(float distance)
	{
		_maximumRenderDistance = distance > 0.0f ? distance : 0.0f;
	}

	void SceneNode::SetUpdatePriority(UpdatePriority priority)
	{
		RN_ASSERT(_sceneInfo == nullptr, "SetUpdatePriority() must be called before adding the node to the scene.");

		WillUpdate(ChangeSet::UpdatePriority);
		_updatePriority = priority;
		DidUpdate(ChangeSet::UpdatePriority);
	}

	void SceneNode::SetRenderPriority(int32 priority)
	{
		if(_renderPriority == priority) return;
		
		WillUpdate(ChangeSet::RenderPriority);
		_renderPriority = priority;

		if(_sceneInfo && !_scheduledForRemovalFromScene)
		{
			//Readd to scene to update the render priority
			_sceneInfo->GetScene()->RemoveNode(this);
			_sceneInfo->GetScene()->AddNode(this);
		}

		DidUpdate(ChangeSet::RenderPriority);
	}

	void SceneNode::SetBoundingBox(const AABB &boundingBox, bool calculateBoundingSphere)
	{
		WillUpdate(ChangeSet::Generic);
		const Vector3 position(boundingBox.position);
		_boundingBox = AABB(position + boundingBox.minExtend, position + boundingBox.maxExtend);

		if(calculateBoundingSphere)
			_boundingSphere = Sphere(_boundingBox);

		_boundsDirty = true;
		DidUpdate(ChangeSet::Generic);
	}

	void SceneNode::SetBoundingSphere(const Sphere &boundingSphere)
	{
		WillUpdate(ChangeSet::Generic);
		_boundingSphere = Sphere(Vector3(boundingSphere.position) + boundingSphere.offset, boundingSphere.radius);
		_boundsDirty = true;
		DidUpdate(ChangeSet::Generic);
	}

	void SceneNode::LookAt(const PositionType &target, bool keepUpAxis)
	{
		const Vector3 direction(GetWorldPosition() - target);

		RN::Quaternion rotation;
		rotation = Quaternion::WithLookAt(direction, GetUp(), keepUpAxis);

		SetWorldRotation(rotation);
	}

	void SceneNode::SetWorldPosition(const PositionType &pos)
	{
		if(_parent)
		{
			const PositionType parentPosition = _parent->GetWorldPosition();
			const Matrix inverseParent = _parent->GetInverseWorldTransform(parentPosition);
			SetPosition(TransformLinearPosition(inverseParent, pos - parentPosition));
			return;
		}

		SetPosition(pos);
	}

	void SceneNode::SetWorldTransform(const PositionType &pos, const Quaternion &rot)
	{
		PositionType localPosition;
		Quaternion localRotation;
		if(_parent)
		{
			const PositionType parentPosition = _parent->GetWorldPosition();
			const Matrix inverseParent = _parent->GetInverseWorldTransform(parentPosition);
			localPosition = TransformLinearPosition(inverseParent, pos - parentPosition);
			localRotation = rot / _parent->GetWorldRotation();
		}
		else
		{
			localPosition = pos;
			localRotation = rot;
		}

#if RN_ENABLE_UNIVERSE_SCALE
		const bool rotationChanged = _rotation->x != localRotation.x ||
			_rotation->y != localRotation.y ||
			_rotation->z != localRotation.z ||
			_rotation->w != localRotation.w;
#endif
		WillUpdate(ChangeSet::Position);

		_position = localPosition;
		_rotation = localRotation;
		_euler = _rotation->GetEulerAngle();
#if RN_ENABLE_UNIVERSE_SCALE
		if(rotationChanged)
			InvalidateLinearTransform();
#endif
		DidUpdate(ChangeSet::Position);
	}

	Matrix SceneNode::GetWorldTransform(const PositionType &renderOrigin) const
	{
		UpdateInternalTransformData();

#if !RN_ENABLE_UNIVERSE_SCALE
		if(renderOrigin.x == 0.0f && renderOrigin.y == 0.0f && renderOrigin.z == 0.0f)
			return _worldTransform;
#endif

		UpdateInternalData();
		Matrix transform = _worldTransform;
		const Vector3 position(_worldPosition - renderOrigin);
		transform.m[12] = position.x;
		transform.m[13] = position.y;
		transform.m[14] = position.z;
		return transform;
	}

	Matrix SceneNode::GetInverseWorldTransform(const PositionType &renderOrigin) const
	{
		UpdateInternalInverseTransformData();

#if !RN_ENABLE_UNIVERSE_SCALE
		if(renderOrigin.x == 0.0f && renderOrigin.y == 0.0f && renderOrigin.z == 0.0f)
			return _inverseWorldTransform;
#endif

		UpdateInternalData();
		Matrix transform = _inverseWorldTransform;
		const Vector3 position(_worldPosition - renderOrigin);
		transform.m[12] = -(transform.m[0] * position.x + transform.m[4] * position.y + transform.m[8] * position.z);
		transform.m[13] = -(transform.m[1] * position.x + transform.m[5] * position.y + transform.m[9] * position.z);
		transform.m[14] = -(transform.m[2] * position.x + transform.m[6] * position.y + transform.m[10] * position.z);
		return transform;
	}

	uint64 SceneNode::GetTransformVersion() const
	{
		return _transformVersion;
	}

	SceneNode::Flags SceneNode::RemoveFlags(Flags flags)
	{
		WillUpdate(ChangeSet::Flags);
		return _flags.fetch_and(~flags, std::memory_order_acq_rel) & ~flags;
		DidUpdate(ChangeSet::Flags);
	}
	SceneNode::Flags SceneNode::AddFlags(Flags flags)
	{
		WillUpdate(ChangeSet::Flags);
		return _flags.fetch_or(flags, std::memory_order_acq_rel) | flags;
		DidUpdate(ChangeSet::Flags);
	}

	// -------------------
	// MARK: -
	// MARK: Scene
	// -------------------

	void SceneNode::UpdateSceneInfo(SceneInfo *sceneInfo)
	{
		WillUpdate(ChangeSet::World);

		if(_sceneInfo)
		{
			Scene *scene = _sceneInfo->GetScene();
			_children->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {
				if(node->_sceneInfo != nullptr)
					scene->RemoveNode(node);
			});
		}

		SafeRelease(_sceneInfo);
		_sceneInfo = sceneInfo;
		SafeRetain(_sceneInfo);

		if(_sceneInfo)
		{
			_children->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {
				sceneInfo->GetScene()->AddNode(node);
			});
		}

		DidUpdate(ChangeSet::World);
	}

	void SceneNode::__CompleteAttachmentWithScene(SceneInfo *sceneInfo)
	{
		if((!_scheduledForRemovalFromScene && (_sceneInfo && sceneInfo && _sceneInfo->GetScene() == sceneInfo->GetScene())) || (!_sceneInfo && !sceneInfo))
			return;

		if(_sceneInfo && !_scheduledForRemovalFromScene)
		{
			_sceneInfo->GetScene()->RemoveNode(this);
		}

		if(sceneInfo)
		{
			//If not removed from scene yet, but _scheduledForRemovalFromScene is true, it will just stay where it is (and is removed from the deletion queue)!
			sceneInfo->GetScene()->AddNode(this);
		}
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
		child->__CompleteAttachmentWithScene(_sceneInfo);

		DidAddChild(child);
	}

	void SceneNode::RemoveChild(SceneNode *child)
	{
		if(child->_parent == this)
		{
			WillRemoveChild(child);
			child->WillUpdate(ChangeSet::Parent);

			child->Retain();
			child->_parent = nullptr;

			_children->RemoveObject(child);

			child->DidUpdate(ChangeSet::Parent);
			child->__CompleteAttachmentWithScene(nullptr);

			DidRemoveChild(child);

			child->Release();
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
		WillUpdate(ChangeSet::Attachments);

		if(!_attachments)
			_attachments = new Array();

		_attachments->AddObject(attachment);
		attachment->_node = this;

		DidUpdate(ChangeSet::Attachments);
	}

	void SceneNode::RemoveAttachment(SceneNodeAttachment *attachment)
	{
		WillUpdate(ChangeSet::Attachments);

		attachment->_node = nullptr;
		attachment->Retain();
		_attachments->RemoveObject(attachment);
		attachment->__DidUpdate(ChangeSet::Attachments); //Usually gets called by the SceneNode::DidUpdate() below, but not if removed from the attachments
		attachment->Release();

		DidUpdate(ChangeSet::Attachments);
	}

	const Array *SceneNode::GetAttachments() const
	{
		return _attachments;
	}

	const Array *SceneNode::GetChildren() const
	{
		return _children;
	}

	SceneNode *SceneNode::GetParent() const
	{
		return _parent;
	}

	void SceneNode::Traverse(const std::function<void(SceneNode *)> &callback)
	{
		callback(this);

		Object **children = const_cast<Object **>(_children->GetData());

		for (size_t i = 0; i < _children->GetCount(); i++) {
			static_cast<SceneNode *>(children[i])->Traverse(callback);
		}
	}

	// -------------------
	// MARK: -
	// MARK: Rendering
	// ------------------

	bool SceneNode::CanRenderUtil(Renderer *renderer, Camera *camera) const
	{
		if((_renderGroup & camera->GetRenderGroup()) == 0)
			return false;

		uint32 flags = _flags.load(std::memory_order_acquire);
		if(flags & Flags::Hidden)
			return false;

		if(flags & Flags::NoCulling)
			return true;

		return camera->InFrustum(GetBoundingSphere(), _maximumRenderDistance);
	}

	bool SceneNode::CanRender(Renderer *renderer, Camera *camera) const
	{
		return false;
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

		if(_attachments)
		{
			for(size_t i = 0; i < _attachments->GetCount(); i += 1)
			{
				static_cast<SceneNodeAttachment *>(_attachments->GetObjectAtIndex(i))->__WillUpdate(changeSet);
			}
		}
	}

	void SceneNode::DidUpdate(ChangeSet changeSet)
	{
		if(changeSet & ChangeSet::Position)
		{
			_transformVersion += 1;

			for(size_t i = 0; i < _children->GetCount(); i += 1)
			{
				static_cast<SceneNode *>(_children->GetObjectAtIndex(i))->DidUpdate(ChangeSet::Position);
			}
		}

		if(changeSet & ChangeSet::Parent)
		{
			_transformVersion += 1;
#if RN_ENABLE_UNIVERSE_SCALE
			_linearTransformVersion += 1;
#endif

			for(size_t i = 0; i < _children->GetCount(); i += 1)
			{
				static_cast<SceneNode *>(_children->GetObjectAtIndex(i))->DidUpdate(ChangeSet::Parent);
			}
		}

		if(_parent)
			_parent->ChildDidUpdate(this, changeSet);

		if(_attachments)
		{
			for(size_t i = 0; i < _attachments->GetCount(); i += 1)
			{
				static_cast<SceneNodeAttachment *>(_attachments->GetObjectAtIndex(i))->__DidUpdate(changeSet);
			}
		}
	}

	PositionType SceneNode::TransformLinearPosition(const Matrix &transform, const PositionType &position)
	{
		return PositionType(
			transform.m[0] * position.x + transform.m[4] * position.y + transform.m[8] * position.z,
			transform.m[1] * position.x + transform.m[5] * position.y + transform.m[9] * position.z,
			transform.m[2] * position.x + transform.m[6] * position.y + transform.m[10] * position.z);
	}

	void SceneNode::UpdateInternalData() const
	{
		const uint64 transformVersion = _transformVersion;
		if(_cachedWorldDataVersion != transformVersion)
		{
			const PositionType position = _position;
			if(_parent)
			{
				_parent->UpdateInternalData();
				_parent->UpdateInternalTransformData();

				const PositionType localOffset = TransformLinearPosition(_parent->_worldTransform, position);
				_worldPosition = _parent->_worldPosition + localOffset;
				_worldRotation = _parent->_worldRotation * _rotation;
				_worldScale = _parent->_worldScale * _scale;
				_worldEuler = _parent->_worldEuler + _euler;
			}
			else
			{
				_worldPosition = _position;
				_worldRotation = _rotation;
				_worldScale = _scale;
				_worldEuler = _euler;
			}

			_cachedWorldDataVersion = transformVersion;
		}
	}

	void SceneNode::UpdateInternalTransformData() const
	{
		const uint64 transformVersion = GetTransformCacheVersion();
		if(_cachedMatrixVersion != transformVersion)
		{
#if RN_ENABLE_UNIVERSE_SCALE
			_localTransform = Matrix();
#else
			const Vector3 position(_position);
			_localTransform = Matrix::WithTranslation(position);
#endif
			_localTransform.Rotate(_rotation);
			_localTransform.Scale(_scale);

			if(_parent)
			{
				_parent->UpdateInternalTransformData();
				_worldTransform = _parent->_worldTransform * _localTransform;
			}
			else
			{
				_worldTransform = _localTransform;
			}

			_cachedMatrixVersion = transformVersion;
		}
	}

	void SceneNode::UpdateInternalInverseTransformData() const
	{
		const uint64 transformVersion = GetTransformCacheVersion();
		if(_cachedInverseMatrixVersion != transformVersion)
		{
			_inverseLocalTransform = Matrix::WithScaling(_scale != 0.0f ? (Vector3(1.0f, 1.0f, 1.0f) / _scale) : Vector3(0.0f, 0.0f, 0.0f));
			_inverseLocalTransform.Rotate(_rotation->GetConjugated());
#if !RN_ENABLE_UNIVERSE_SCALE
			const Vector3 position(_position);
			_inverseLocalTransform.Translate(position * -1.0f);
#endif
			if(_parent)
			{
				_parent->UpdateInternalInverseTransformData();
				_inverseWorldTransform = _inverseLocalTransform * _parent->_inverseWorldTransform;
			}
			else
			{
				_inverseWorldTransform = _inverseLocalTransform;
			}

			_cachedInverseMatrixVersion = transformVersion;
		}
	}

	void SceneNode::UpdateInternalBoundsData() const
	{
		UpdateInternalData();

		const uint64 transformVersion = GetTransformCacheVersion();
		if(_boundsDirty || _cachedBoundsVersion != transformVersion)
		{
			_transformedBoundingBox = _boundingBox;
			_transformedBoundingBox *= _worldScale;
			_transformedBoundingBox.Rotate(_worldRotation);

			_transformedBoundingSphere = _boundingSphere;
			_transformedBoundingSphere *= _worldScale;
			_transformedBoundingSphere.SetRotation(_worldRotation);

			_cachedBoundsVersion = transformVersion;
			_boundsDirty = false;
		}
	}
} // namespace RN
