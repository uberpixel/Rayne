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
#include "../Data/RNIntrusiveList.h"
#include "../Objects/RNObject.h"
#include "../Math/RNMatrix.h"
#include "../Math/RNQuaternion.h"
#include "../Math/RNVector.h"
#include "../Math/RNAABB.h"
#include "../Math/RNSphere.h"
#include "../Base/RNSignal.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNKVOImplementation.h"

namespace RN
{
	class Scene;
	class SceneInfo;
	class Camera;
	class Renderer;
	class SceneNodeAttachment;

	class SceneNode : public Object
	{
	public:
		friend class Scene;

		enum class Priority
		{
			UpdateEarly,
			UpdateNormal,
			UpdateLate
		};

		RN_OPTIONS(Flags, uint32,
				   Static       = (1 << 0),
				   Hidden       = (1 << 1),
				   
				   DrawEarly    = (1 << 2),
				   DrawLate     = (1 << 3),
				   DrawLater	= (1 << 4),

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

		void Translate(const Vector3 &trans);
		void TranslateLocal(const Vector3 &trans);
		void Scale(const Vector3 &scal);
		void Rotate(const Vector3 &rot);
		void Rotate(const Quaternion &rot);

		RNAPI void SetFlags(Flags flags);
		RNAPI void SetTag(Tag tag);

		RNAPI void SetRenderGroup(uint8 group);
		RNAPI void SetCollisionGroup(uint8 group);

		virtual void SetPosition(const Vector3 &pos);
		virtual void SetScale(const Vector3 &scal);
		virtual void SetRotation(const Quaternion &rot);

		virtual void SetWorldPosition(const Vector3 &pos);
		virtual void SetWorldScale(const Vector3 &scal);
		virtual void SetWorldRotation(const Quaternion &rot);

		RNAPI void SetBoundingBox(const AABB &boundingBox, bool calculateBoundingSphere=true);
		RNAPI void SetBoundingSphere(const Sphere &boundingSphere);

		RNAPI void SetPriority(Priority priority);

		RNAPI bool HasFlags(Flags flags) const;
		RNAPI Flags RemoveFlags(Flags flags);
		RNAPI Flags AddFlags(Flags flags);

		uint64 GetUID() const { return _uid; }
		uint64 GetLID() const { return _lid; }

		Tag GetTag() const { return _tag; }

		Vector3 GetForward() const;
		Vector3 GetUp() const;
		Vector3 GetRight() const;

		Vector3 GetWorldPosition() const;
		Vector3 GetWorldScale() const;
		Vector3 GetWorldEulerAngle() const;
		Quaternion GetWorldRotation() const;

		AABB GetBoundingBox() const;
		Sphere GetBoundingSphere() const;

		const Vector3 &GetPosition() const { return _position; }
		const Vector3 &GetScale() const { return _scale; }
		const Vector3 &GetEulerAngle() const { return _euler; }
		const Quaternion &GetRotation() const { return _rotation; }

		RNAPI void LookAt(const RN::Vector3 &target, bool keepUpAxis=false);

		RNAPI void AddChild(SceneNode *child);
		RNAPI void RemoveChild(SceneNode *child);
		RNAPI void RemoveFromParent();

		RNAPI void AddAttachment(SceneNodeAttachment *attachment);
		RNAPI void RemoveAttachment(SceneNodeAttachment *attachment);

		RNAPI SceneNode *GetParent() const;
		SceneInfo *GetSceneInfo() const { return _sceneInfo; };
		Priority GetPriority() const { return _priority; }
		Flags GetFlags() const { return _flags.load(); }

		uint8 GetRenderGroup() const { return _renderGroup; };
		uint8 GetCollisionGroup() const { return _collisionGroup; };

		RNAPI const Array *GetAttachments() const;
		RNAPI const Array *GetChildren() const;

		RNAPI Matrix GetWorldTransform() const;
		RNAPI Matrix GetTransform() const;

		RNAPI virtual bool CanRender(Renderer *renderer, Camera *camera) const;
		RNAPI virtual void Render(Renderer *renderer, Camera *camera) const;

		RNAPI virtual void Update(float delta);
		
		IntrusiveList<SceneNode>::Member _sceneEntry; //TODO: Make private but keep accessible to user made scene implementations
		RNAPI void UpdateSceneInfo(SceneInfo *sceneInfo); //TODO: Make private but keep accessible to user made scene implementations

	protected:
		RNAPI virtual void WillUpdate(ChangeSet changeSet);
		RNAPI virtual void DidUpdate(ChangeSet changeSet);

		virtual void ChildDidUpdate(SceneNode *child, ChangeSet changes) {}
		virtual void ChildWillUpdate(SceneNode *child, ChangeSet changes) {}
		virtual void WillAddChild(SceneNode *child) {}
		virtual void DidAddChild(SceneNode *child)  {}
		virtual void WillRemoveChild(SceneNode *child) {}
		virtual void DidRemoveChild(SceneNode *child) {}

	private:
		void Initialize();
		RNAPI void UpdateInternalData() const;

		void __RemoveSceneInfo();
		void __CompleteAttachmentWithScene(SceneInfo *sceneInfo);

		AABB _boundingBox;
		Sphere _boundingSphere;

		SceneNode *_parent;
		Array *_children;

		Priority _priority;
		std::atomic<uint32> _flags;

		uint8 _renderGroup;
		uint8 _collisionGroup;

		uint64 _uid;
		uint64 _lid;

		SceneInfo *_sceneInfo;
		bool _scheduledForRemovalFromScene;

		ObservableScalar<Tag, SceneNode> _tag;

		ObservableValue<Vector3, SceneNode> _position;
		ObservableValue<Vector3, SceneNode> _scale;
		ObservableValue<Quaternion, SceneNode> _rotation;
		Vector3 _euler;

		Array *_attachments;

		mutable bool _updated;
		mutable Vector3 _worldPosition;
		mutable Quaternion _worldRotation;
		mutable Vector3 _worldScale;
		mutable Vector3 _worldEuler;

		mutable Matrix _worldTransform;
		mutable Matrix _localTransform;

		mutable AABB _transformedBoundingBox;
		mutable Sphere _transformedBoundingSphere;

		__RNDeclareMetaInternal(SceneNode)
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
		_euler = _rotation->GetEulerAngle();

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
		Vector3 tempPosition = pos - _parent->GetWorldPosition();
		Quaternion tempRotation = Quaternion()/_parent->GetWorldRotation();
		_position = tempRotation.GetRotatedVector(tempPosition);
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

	RN_INLINE AABB SceneNode::GetBoundingBox() const
	{
		UpdateInternalData();
		return AABB(_transformedBoundingBox);
	}

	RN_INLINE Sphere SceneNode::GetBoundingSphere() const
	{
		UpdateInternalData();
		return Sphere(_transformedBoundingSphere);
	}
}


#endif /* __RAYNE_SCENENODE_H__ */
