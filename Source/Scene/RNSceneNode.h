//
//  RNSceneNode.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_SCENENODE_H__
#define __RAYNE_SCENENODE_H__

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Math/RNMatrix.h"
#include "../Math/RNQuaternion.h"
#include "../Math/RNVector.h"
#include "../Base/RNSignal.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNKVOImplementation.h"

namespace RN
{
	class SceneNode : public Object
	{
	public:
		enum class Priority
		{
			UpdateEarly,
			UpdateDontCare,
			UpdateLate
		};

		RN_OPTIONS(Flags, uint32,
				   DrawLate     = (1 << 0),
				   Static       = (1 << 1),
				   Hidden       = (1 << 2),
				   HideChildren = (1 << 3),

				   HideInEditor        = (1 << 10),
				   UndeletableInEditor = (1 << 11),
				   LockedInEditor      = (1 << 12),

				   NoSave  = (1 << 4),
				   Mutated = (1 << 5));

		RN_OPTIONS(ChangeSet, uint32,
				   Generic = (1 << 0),
				   Flags = (1 << 1),
				   Position = (1 << 2),
				   Dependencies = (1 << 3),
				   Priority = (1 << 4),
				   Parent = (1 << 5),
				   Attachments = (1 << 6),
				   World = (1 << 7),
				   Tag = (1 << 8));

		RNAPI SceneNode();
		RNAPI SceneNode(const Vector3 &position);
		RNAPI SceneNode(const Vector3 &position, const Quaternion &rotation);
		RNAPI SceneNode(const SceneNode *other);
		RNAPI SceneNode(Deserializer *deserializer);
		RNAPI ~SceneNode() override;

		RNAPI void Serialize(Serializer *serializer) const override;

		RNAPI void Translate(const Vector3 &trans);
		RNAPI void TranslateLocal(const Vector3 &trans);
		RNAPI void Scale(const Vector3 &scal);
		RNAPI void Rotate(const Vector3 &rot);
		RNAPI void Rotate(const Quaternion &rot);

		RNAPI void RemoveFromWorld();

		RNAPI void SetFlags(Flags flags);
		RNAPI void SetTag(Tag tag);

		RNAPI void SetRenderGroup(uint8 group);
		RNAPI void SetCollisionGroup(uint8 group);

		RNAPI virtual void SetPosition(const Vector3 &pos);
		RNAPI virtual void SetScale(const Vector3 &scal);
		RNAPI virtual void SetRotation(const Quaternion &rot);

		RNAPI virtual void SetWorldPosition(const Vector3 &pos);
		RNAPI virtual void SetWorldScale(const Vector3 &scal);
		RNAPI virtual void SetWorldRotation(const Quaternion &rot);

		RNAPI void SetPriority(Priority priority);
		RNAPI void SetDebugName(const std::string &name);

		RNAPI uint64 GetUID() const { return _uid; }
		RNAPI uint64 GetLID() const { return _lid; }

		RNAPI Tag GetTag() const { return _tag; }

		RNAPI Vector3 GetForward() const;
		RNAPI Vector3 GetUp() const;
		RNAPI Vector3 GetRight() const;

		RNAPI Vector3 GetWorldPosition() const;
		RNAPI Vector3 GetWorldScale() const;
		RNAPI Vector3 GetWorldEulerAngle() const;
		RNAPI Quaternion GetWorldRotation() const;

		RNAPI const Vector3 &GetPosition() const { return _position; }
		RNAPI const Vector3 &GetScale() const { return _scale; }
		RNAPI const Vector3 &GetEulerAngle() const { return _euler; }
		RNAPI const Quaternion &GetRotation() const { return _rotation; }

		RNAPI const std::string &GetDebugName() { return _debugName; }

		RNAPI void LookAt(const RN::Vector3 &target, bool keepUpAxis=false);

		RNAPI void AddChild(SceneNode *child);
		RNAPI void RemoveChild(SceneNode *child);
		RNAPI void RemoveFromParent();

		RNAPI void SetAction(const std::function<void (SceneNode *, float)>& action);

		RNAPI SceneNode *GetParent() const;
		RNAPI Priority GetPriority() const { return _priority; }
		RNAPI Flags GetFlags() const { return _flags.load(); }

		RNAPI uint8 GetRenderGroup() const {return renderGroup;};
		RNAPI uint8 GetCollisionGroup() const {return collisionGroup;};

		RNAPI const Array *GetChildren() const;
		RNAPI bool HasChildren() const;

		RNAPI Matrix GetWorldTransform() const;
		RNAPI Matrix GetTransform() const;

		virtual void Update(float delta)
		{
			if(_action)
				_action(this, delta);
		}

		virtual void UpdateEditMode(float delta)
		{

		}

	protected:
		RNAPI virtual void WillUpdate(ChangeSet changeSet);
		RNAPI virtual void DidUpdate(ChangeSet changeSet);

		RNAPI virtual void ChildDidUpdate(SceneNode *child, ChangeSet changes) {}
		RNAPI virtual void ChildWillUpdate(SceneNode *child, ChangeSet changes) {}
		RNAPI virtual void WillAddChild(SceneNode *child) {}
		RNAPI virtual void DidAddChild(SceneNode *child)  {}
		RNAPI virtual void WillRemoveChild(SceneNode *child) {}
		RNAPI virtual void DidRemoveChild(SceneNode *child) {}

		Observable<Vector3, SceneNode> _position;
		Observable<Vector3, SceneNode> _scale;
		Observable<Quaternion, SceneNode> _rotation;
		Vector3 _euler;

		Signal<void (SceneNode *)> _cleanUpSignal;

	private:
		void Initialize();
		void UpdateInternalData() const;

		SceneNode *_parent;
		Array _children;

		Priority _priority;
		std::atomic<uint32> _flags;

		uint8 renderGroup;
		uint8 collisionGroup;

		uint64 _uid;
		uint64 _lid;

		Observable<Tag, SceneNode> _tag;

		std::function<void (SceneNode *, float)> _action;
		std::string _debugName;

		mutable bool _updated;
		mutable Vector3 _worldPosition;
		mutable Quaternion _worldRotation;
		mutable Vector3 _worldScale;
		mutable Vector3 _worldEuler;

		mutable Matrix _worldTransform;
		mutable Matrix _localTransform;

		RNDeclareMeta(SceneNode)
	};

	RNObjectClass(SceneNode)

	RN_INLINE void SceneNode::Translate(const Vector3 &trans)
	{
		WillUpdate(ChangeSet::Position);

		_position += trans;

		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::Scale(const Vector3 &scal)
	{
		WillUpdate(ChangeSet::Position);

		_scale += scal;

		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::Rotate(const Vector3 &rot)
	{
		WillUpdate(ChangeSet::Position);

		_euler += rot;
		_rotation = Quaternion(_euler);

		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::Rotate(const Quaternion &rot)
	{
		WillUpdate(ChangeSet::Position);

		_rotation *= rot;

		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::TranslateLocal(const Vector3 &trans)
	{
		WillUpdate(ChangeSet::Position);
		_position += _rotation->GetRotatedVector(trans);
		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::SetPosition(const Vector3 &pos)
	{
		WillUpdate(ChangeSet::Position);
		_position = pos;
		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::SetScale(const Vector3 &scal)
	{
		WillUpdate(ChangeSet::Position);
		_scale = scal;
		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::SetRotation(const Quaternion &rot)
	{
		WillUpdate(ChangeSet::Position);

		_euler    = rot.GetEulerAngle();
		_rotation = rot;

		DidUpdate(ChangeSet::Position);
	}


	RN_INLINE void SceneNode::SetWorldPosition(const Vector3 &pos)
	{
		if(!_parent)
		{
			SetPosition(pos);
			return;
		}

		WillUpdate(ChangeSet::Position);

		Quaternion temp;
		temp = temp / _parent->GetWorldRotation();

		_position = temp.GetRotatedVector(pos) - temp.GetRotatedVector(GetWorldPosition());

		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::SetWorldScale(const Vector3 &scal)
	{
		if(!_parent)
		{
			SetScale(scal);
			return;
		}

		WillUpdate(ChangeSet::Position);

		if(_parent && !Math::Compare(_parent->GetWorldScale().GetMin(), 0.0f))
		{
			_scale = scal / _parent->GetWorldScale();
		}
		else
		{
			_scale = scal;
		}

		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::SetWorldRotation(const Quaternion &rot)
	{
		if(!_parent)
		{
			SetRotation(rot);
			return;
		}

		WillUpdate(ChangeSet::Position);

		_rotation = rot / _parent->GetWorldRotation();
		_euler = _rotation->GetEulerAngle();

		DidUpdate(ChangeSet::Position);
	}


	RN_INLINE Vector3 SceneNode::GetForward() const
	{
		Vector3 forward = GetWorldRotation().GetRotatedVector(Vector3(0.0, 0.0, -1.0));
		return forward;
	}

	RN_INLINE Vector3 SceneNode::GetUp() const
	{
		Vector3 up = GetWorldRotation().GetRotatedVector(Vector3(0.0, 1.0, 0.0));
		return up;
	}

	RN_INLINE Vector3 SceneNode::GetRight() const
	{
		Vector3 right = GetWorldRotation().GetRotatedVector(Vector3(1.0, 0.0, 0.0));
		return right;
	}

	RN_INLINE Vector3 SceneNode::GetWorldPosition() const
	{
		UpdateInternalData();
		return Vector3(_worldPosition);
	}
	RN_INLINE Vector3 SceneNode::GetWorldScale() const
	{
		UpdateInternalData();
		return Vector3(_worldScale);
	}
	RN_INLINE Vector3 SceneNode::GetWorldEulerAngle() const
	{
		UpdateInternalData();
		return Vector3(_worldEuler);
	}
	RN_INLINE Quaternion SceneNode::GetWorldRotation() const
	{
		UpdateInternalData();
		return Quaternion(_worldRotation);
	}

	RN_INLINE Matrix SceneNode::GetTransform() const
	{
		UpdateInternalData();
		return Matrix(_localTransform);
	}

	RN_INLINE Matrix SceneNode::GetWorldTransform() const
	{
		UpdateInternalData();
		return Matrix(_worldTransform);
	}

	RN_INLINE void SceneNode::UpdateInternalData() const
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

			_updated = false;

			size_t count = _children.GetCount();

			for(size_t i = 0; i < count; i ++)
			{
				SceneNode *child = static_cast<SceneNode *>(_children[i]);
				child->_updated = true;
			}
		}
	}
}


#endif /* __RAYNE_SCENENODE_H__ */
