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
#include "../Base/RNSignal.h"
#include "../Data/RNIntrusiveList.h"
#include "../Math/RNAABB.h"
#include "../Math/RNMatrix.h"
#include "../Math/RNQuaternion.h"
#include "../Math/RNSphere.h"
#include "../Math/RNVector.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNKVOImplementation.h"
#include "../Objects/RNObject.h"

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

		enum class UpdatePriority
		{
			UpdateEarliest,
			UpdateEarly,
			UpdateNormal,
			UpdateLate,
			UpdateNever //Update will never be called for these
		};

		enum RenderPriority : int32
		{
			RenderEarly = -100000,
			RenderNormal = 0,
			RenderLate = 100000,
			RenderSky = 200000,
			RenderTransparent = 300000,
			RenderUI = 1000000
		};

		RN_OPTIONS(Flags, uint32,
				   Static = (1 << 0),
				   Hidden = (1 << 1),
				   NoCulling = (1 << 2),

				   Occluder = (1 << 3),

				   HideInEditor = (1 << 10),
				   UndeletableInEditor = (1 << 11),
				   LockedInEditor = (1 << 12),

				   NoSave = (1 << 4),
				   Mutated = (1 << 5));

		RN_OPTIONS(ChangeSet, uint32,
				   Generic = (1 << 0),
				   Flags = (1 << 1),
				   Position = (1 << 2),
				   Dependencies = (1 << 3),
				   UpdatePriority = (1 << 4),
				   RenderPriority = (1 << 5),
				   Parent = (1 << 6),
				   Attachments = (1 << 7),
				   World = (1 << 8),
				   Tag = (1 << 9));

		RNAPI SceneNode();
		RNAPI SceneNode(const PositionType &position);
		RNAPI SceneNode(const PositionType &position, const Quaternion &rotation);
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

		RNAPI void SetRenderGroup(uint16 group);
		RNAPI void SetCollisionGroup(uint8 group);
		RNAPI void SetMaximumRenderDistance(float distance);

		virtual void SetPosition(const PositionType &pos);
		virtual void SetScale(const Vector3 &scal);
		virtual void SetRotation(const Quaternion &rot);

		RNAPI virtual void SetWorldPosition(const PositionType &pos);
		virtual void SetWorldScale(const Vector3 &scal);
		virtual void SetWorldRotation(const Quaternion &rot);
		RNAPI virtual void SetWorldTransform(const PositionType &pos, const Quaternion &rot);

		RNAPI void SetBoundingBox(const AABB &boundingBox, bool calculateBoundingSphere = true);
		RNAPI void SetBoundingSphere(const Sphere &boundingSphere);

		RNAPI void SetUpdatePriority(UpdatePriority priority);
		RNAPI void SetRenderPriority(int32 priority);

		RNAPI bool HasFlags(Flags flags) const { return (_flags.load(std::memory_order_acquire) & flags); }
		RNAPI Flags RemoveFlags(Flags flags);
		RNAPI Flags AddFlags(Flags flags);

		uint64 GetUID() const { return _uid; }
		uint64 GetLID() const { return _lid; }

		Tag GetTag() const { return _tag; }

		virtual Vector3 GetForward() const;
		virtual Vector3 GetUp() const;
		virtual Vector3 GetRight() const;

		PositionType GetWorldPosition() const;
		// Float-space matrices relative to the supplied rendering origin.
		RNAPI Matrix GetWorldTransform(const PositionType &renderOrigin = PositionType()) const;
		RNAPI Matrix GetInverseWorldTransform(const PositionType &renderOrigin = PositionType()) const;
		Vector3 GetWorldScale() const;
		Vector3 GetWorldEulerAngle() const;
		Quaternion GetWorldRotation() const;
		RNAPI uint64 GetTransformVersion() const;

		// Bounds use an exact world-space position with float geometry relative to it.
		AABB GetBoundingBox() const;
		Sphere GetBoundingSphere() const;

		const PositionType &GetPosition() const { return _position; }
		const Vector3 &GetScale() const { return _scale; }
		const Vector3 &GetEulerAngle() const { return _euler; }
		const Quaternion &GetRotation() const { return _rotation; }

		RNAPI void LookAt(const PositionType &target, bool keepUpAxis = false);

		RNAPI void AddChild(SceneNode *child);
		RNAPI void RemoveChild(SceneNode *child);
		RNAPI void RemoveFromParent();

		RNAPI void AddAttachment(SceneNodeAttachment *attachment);
		RNAPI void RemoveAttachment(SceneNodeAttachment *attachment);

		RNAPI SceneNode *GetParent() const;
		SceneInfo *GetSceneInfo() const { return _sceneInfo; };
		UpdatePriority GetUpdatePriority() const { return _updatePriority; }
		int32 GetRenderPriority() const { return _renderPriority; }
		Flags GetFlags() const { return _flags.load(); }

		uint16 GetRenderGroup() const { return _renderGroup; };
		uint8 GetCollisionGroup() const { return _collisionGroup; };
		float GetMaximumRenderDistance() const { return _maximumRenderDistance; }

		RNAPI const Array *GetAttachments() const;
		RNAPI const Array *GetChildren() const;

		RNAPI void Traverse(const std::function<void(SceneNode *)> &callback);

		RNAPI Matrix GetTransform() const;
		RNAPI Matrix GetInverseTransform() const;

		RNAPI virtual bool CanRender(Renderer *renderer, Camera *camera) const;
		RNAPI virtual void Render(Renderer *renderer, Camera *camera) const;
		void PrepareForRenderIfNeeded()
		{
			if(RN_EXPECT_FALSE(_needsRenderPreparation))
			{
				_needsRenderPreparation = false;
				PrepareForRender();
			}
		}

		RNAPI virtual void Update(float delta);

		IntrusiveList<SceneNode>::Member _sceneUpdateEntry; //TODO: Make private but keep accessible to user made scene implementations
		IntrusiveList<SceneNode>::Member _sceneRenderEntry; //TODO: Make private but keep accessible to user made scene implementations
		RNAPI void UpdateSceneInfo(SceneInfo *sceneInfo); //TODO: Make private but keep accessible to user made scene implementations

		bool _scheduledForRemovalFromScene; //TODO: Make private but keep accessible to user made scene

	protected:
		RNAPI virtual void WillUpdate(ChangeSet changeSet);
		RNAPI virtual void DidUpdate(ChangeSet changeSet);

		virtual void ChildDidUpdate(SceneNode *child, ChangeSet changes) {}
		virtual void ChildWillUpdate(SceneNode *child, ChangeSet changes) {}
		virtual void WillAddChild(SceneNode *child) {}
		virtual void DidAddChild(SceneNode *child) {}
		virtual void WillRemoveChild(SceneNode *child) {}
		virtual void DidRemoveChild(SceneNode *child) {}

		//Can be used by other classes for basic checks, like the being in the camera frustum and not hidden
		RNAPI virtual bool CanRenderUtil(Renderer *renderer, Camera *camera) const;
		virtual void PrepareForRender() {}
		void SetNeedsRenderPreparation() { _needsRenderPreparation = true; }

	private:
		void Initialize();
		void InvalidateLinearTransform();
		uint64 GetTransformCacheVersion() const;
		static PositionType TransformLinearPosition(const Matrix &transform, const PositionType &position);
		RNAPI void UpdateInternalData() const;
		RNAPI void UpdateInternalTransformData() const;
		RNAPI void UpdateInternalInverseTransformData() const;
		RNAPI void UpdateInternalBoundsData() const;

		void __CompleteAttachmentWithScene(SceneInfo *sceneInfo);

		AABB _boundingBox;
		Sphere _boundingSphere;

		SceneNode *_parent;
		Array *_children;

		UpdatePriority _updatePriority;
		int32 _renderPriority;
		std::atomic<uint32> _flags;

		uint16 _renderGroup;
		uint8 _collisionGroup;
		bool _needsRenderPreparation;
		float _maximumRenderDistance;

		uint64 _uid;
		uint64 _lid;

		SceneInfo *_sceneInfo;

		ObservableScalar<Tag, SceneNode> _tag;

		ObservableValue<PositionType, SceneNode> _position;
		ObservableValue<Vector3, SceneNode> _scale;
		ObservableValue<Quaternion, SceneNode> _rotation;
		Vector3 _euler;

		Array *_attachments;

		uint64 _transformVersion;
#if RN_ENABLE_UNIVERSE_SCALE
		uint64 _linearTransformVersion;
#endif
		mutable bool _boundsDirty;
		mutable uint64 _cachedWorldDataVersion;
		mutable uint64 _cachedMatrixVersion;
		mutable uint64 _cachedInverseMatrixVersion;
		mutable uint64 _cachedBoundsVersion;
		mutable PositionType _worldPosition;
		mutable Quaternion _worldRotation;
		mutable Vector3 _worldScale;
		mutable Vector3 _worldEuler;

		// Complete matrices without universe scale; translation-free linear matrices with it.
		mutable Matrix _worldTransform;
		mutable Matrix _inverseWorldTransform;
		mutable Matrix _localTransform;
		mutable Matrix _inverseLocalTransform;

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
		InvalidateLinearTransform();

		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::Rotate(const Vector3 &rot)
	{
		WillUpdate(ChangeSet::Position);

		_euler += rot;
		_rotation = Quaternion(_euler);
		InvalidateLinearTransform();

		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::Rotate(const Quaternion &rot)
	{
		WillUpdate(ChangeSet::Position);

		_rotation *= rot;
		_euler = _rotation->GetEulerAngle();
		InvalidateLinearTransform();

		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::TranslateLocal(const Vector3 &trans)
	{
		WillUpdate(ChangeSet::Position);
		_position += _rotation->GetRotatedVector(trans);
		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::SetPosition(const PositionType &pos)
	{
		WillUpdate(ChangeSet::Position);
		_position = pos;
		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::SetScale(const Vector3 &scal)
	{
		WillUpdate(ChangeSet::Position);
		_scale = scal;
		InvalidateLinearTransform();
		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::SetRotation(const Quaternion &rot)
	{
		WillUpdate(ChangeSet::Position);

		_euler = rot.GetEulerAngle();
		_rotation = rot;
		InvalidateLinearTransform();

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
		InvalidateLinearTransform();

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
		InvalidateLinearTransform();

		DidUpdate(ChangeSet::Position);
	}

	RN_INLINE void SceneNode::InvalidateLinearTransform()
	{
#if RN_ENABLE_UNIVERSE_SCALE
		_linearTransformVersion += 1;
		for(size_t i = 0; i < _children->GetCount(); i += 1)
		{
			static_cast<SceneNode *>(_children->GetObjectAtIndex(i))->InvalidateLinearTransform();
		}
#endif
	}

	RN_INLINE uint64 SceneNode::GetTransformCacheVersion() const
	{
#if RN_ENABLE_UNIVERSE_SCALE
		return _linearTransformVersion;
#else
		return _transformVersion;
#endif
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

	RN_INLINE PositionType SceneNode::GetWorldPosition() const
	{
		UpdateInternalData();
		return _worldPosition;
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
		UpdateInternalTransformData();

#if RN_ENABLE_UNIVERSE_SCALE
		Matrix transform = _localTransform;
		const Vector3 position(_position);
		transform.m[12] = position.x;
		transform.m[13] = position.y;
		transform.m[14] = position.z;
		return transform;
#else
		return Matrix(_localTransform);
#endif
	}

	RN_INLINE Matrix SceneNode::GetInverseTransform() const
	{
		UpdateInternalInverseTransformData();

#if RN_ENABLE_UNIVERSE_SCALE
		Matrix transform = _inverseLocalTransform;
		const Vector3 position(_position);
		transform.m[12] = -(transform.m[0] * position.x + transform.m[4] * position.y + transform.m[8] * position.z);
		transform.m[13] = -(transform.m[1] * position.x + transform.m[5] * position.y + transform.m[9] * position.z);
		transform.m[14] = -(transform.m[2] * position.x + transform.m[6] * position.y + transform.m[10] * position.z);
		return transform;
#else
		return Matrix(_inverseLocalTransform);
#endif
	}

	RN_INLINE AABB SceneNode::GetBoundingBox() const
	{
		UpdateInternalBoundsData();

		AABB result(_transformedBoundingBox);
		result.position = _worldPosition;
		return result;
	}

	RN_INLINE Sphere SceneNode::GetBoundingSphere() const
	{
		UpdateInternalBoundsData();

		Sphere result(_transformedBoundingSphere);
		result.position = _worldPosition;
		return result;
	}
} // namespace RN


#endif /* __RAYNE_SCENENODE_H__ */
