//
//  RNSceneNode.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCENENODE_H__
#define __RAYNE_SCENENODE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"
#include "RNVector.h"
#include "RNSignal.h"
#include "RNArray.h"
#include "RNAABB.h"
#include "RNSphere.h"
#include "RNKVOImplementation.h"
#include "RNSTL.h"
#include "RNEnum.h"

#define kRNSceneNodeDidUpdateWorldMessage RNCSTR("kRNSceneNodeDidUpdateWorldMessage")

namespace RN
{
	class Renderer;
	class Camera;
	class World;
	
	class RenderingObject;
	class SceneNodeAttachment;
	
	class SceneNode : public Object
	{
	public:
		friend class Renderer;
		friend class World;
		
		enum class Priority
		{
			UpdateEarly,
			UpdateDontCare,
			UpdateLate
		};
		
		struct Flags : public Enum<uint32>
		{
			Flags()
			{}
			Flags(int value) :
				Enum(value)
			{}
			
			enum
			{
				DrawLate     = (1 << 0),
				Static       = (1 << 1),
				Hidden       = (1 << 2),
				HideChildren = (1 << 3),
				
				HideInEditor        = (1 << 10),
				UndeletableInEditor = (1 << 11),
				LockedInEditor      = (1 << 12),
				
				NoSave  = (1 << 4),
				Mutated = (1 << 5)
			};
		};
		
		struct ChangeSet : public Enum<uint32>
		{
			ChangeSet()
			{}
			ChangeSet(int value) :
				Enum(value)
			{}
			
			enum
			{
				Generic = (1 << 0),
				Flags = (1 << 1),
				Position = (1 << 2),
				Dependencies = (1 << 3),
				Priority = (1 << 4),
				Parent = (1 << 5),
				Attachments = (1 << 6),
				World = (1 << 7),
				Tag = (1 << 8)
			};
		};
		
		RNAPI SceneNode();
		RNAPI SceneNode(const Vector3& position);
		RNAPI SceneNode(const Vector3& position, const Quaternion& rotation);
		RNAPI SceneNode(const SceneNode *other);
		RNAPI SceneNode(Deserializer *deserializer);
		RNAPI ~SceneNode() override;
		
		RNAPI void FillRenderingObject(RenderingObject& object) const;
		RNAPI void Serialize(Serializer *serializer) override;
		
		RNAPI void Translate(const Vector3& trans);
		RNAPI void TranslateLocal(const Vector3& trans);
		RNAPI void Scale(const Vector3& scal);
		RNAPI void Rotate(const Vector3& rot);
		RNAPI void Rotate(const Quaternion& rot);
		
		RNAPI void RemoveFromWorld();
		
		RNAPI void SetFlags(Flags flags);
		RNAPI void SetTag(Tag tag);
		
		RNAPI void SetRenderGroup(uint8 group);
		RNAPI void SetCollisionGroup(uint8 group);
		
		RNAPI virtual void SetPosition(const Vector3& pos);
		RNAPI virtual void SetScale(const Vector3& scal);
		RNAPI virtual void SetRotation(const Quaternion& rot);
		
		RNAPI virtual void SetWorldPosition(const Vector3& pos);
		RNAPI virtual void SetWorldScale(const Vector3& scal);
		RNAPI virtual void SetWorldRotation(const Quaternion& rot);
		
		RNAPI void SetBoundingBox(const AABB& boundingBox, bool calculateBoundingSphere=true);
		RNAPI void SetBoundingSphere(const Sphere& boundingSphere);
		
		RNAPI void SetPriority(Priority priority);
		RNAPI void SetDebugName(const std::string& name);
		
		RNAPI virtual bool IsVisibleInCamera(Camera *camera);
		RNAPI virtual void Render(Renderer *renderer, Camera *camera);
		
		RNAPI uint64 GetUID() const { return _uid; }
		RNAPI uint64 GetLID() const { return _lid; }
		
		RNAPI Tag GetTag() const { return _tag; }
		
		RNAPI Vector3 GetPosition() const;
		RNAPI Vector3 GetScale() const;
		RNAPI Vector3 GetEulerAngle() const;
		RNAPI Quaternion GetRotation() const;
		
		RNAPI Vector3 GetForward() const;
		RNAPI Vector3 GetUp() const;
		RNAPI Vector3 GetRight() const;
		
		RNAPI Vector3 GetWorldPosition() const;
		RNAPI Vector3 GetWorldScale() const;
		RNAPI Vector3 GetWorldEulerAngle() const;
		RNAPI Quaternion GetWorldRotation() const;
		
		RNAPI const Vector3 &GetPosition_NoLock() const { return _position; }
		RNAPI const Vector3 &GetScale_NoLock() const { return _scale; }
		RNAPI const Vector3 &GetEulerAngle_NoLock() const { return _euler; }
		RNAPI const Quaternion &GetRotation_NoLock() const { return _rotation; }
		
		RNAPI const Vector3 &GetWorldPosition_NoLock() const;
		RNAPI const Vector3 &GetWorldScale_NoLock() const;
		RNAPI const Vector3 &GetWorldEulerAngle_NoLock() const;
		RNAPI const Quaternion &GetWorldRotation_NoLock() const;
		
		RNAPI AABB GetBoundingBox() const;
		RNAPI Sphere GetBoundingSphere() const;
		
		RNAPI const std::string& GetDebugName() { return _debugName; }
		
		RNAPI void LookAt(const RN::Vector3 &target, bool keepUpAxis=false);
		
		RNAPI void AddChild(SceneNode *child);
		RNAPI void RemoveChild(SceneNode *child);
		RNAPI void RemoveFromParent();
		
		RNAPI void SetAction(const std::function<void (SceneNode *, float)>& action);
		RNAPI void AddDependency(SceneNode *dependency);
		RNAPI void RemoveDependency(SceneNode *dependency);
		
		RNAPI void AddAttachment(SceneNodeAttachment *attachment);
		RNAPI void RemoveAttachment(SceneNodeAttachment *attachment);
		RNAPI SceneNodeAttachment *GetAttachment(MetaClassBase *metaClass) const;
		RNAPI Array *GetAttachments() const;
		
		template<class T>
		T *GetAttachment() const
		{
			SceneNodeAttachment *attachment = GetAttachment(T::MetaClass());
			return static_cast<T *>(attachment);
		}
		
		RNAPI SceneNode *GetParent() const;
		RNAPI FrameID GetLastFrame() const { return _lastFrame; }
		RNAPI World *GetWorld() const { return _worldInserted ? _world : nullptr; }
		RNAPI Priority GetPriority() const { return _priority; }
		RNAPI Flags GetFlags() const { return _flags.load(); }
		
		RNAPI uint8 GetRenderGroup() const {return renderGroup;};
		RNAPI uint8 GetCollisionGroup() const {return collisionGroup;};
		
		RNAPI const Array *GetChildren() const;
		RNAPI bool HasChildren() const;
		
		RNAPI virtual class Hit CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode = Hit::HitMode::IgnoreNone);
		
		RNAPI Matrix GetWorldTransform() const;
		RNAPI Matrix GetTransform() const;
		
		virtual void Update(float delta)
		{
			if(_action)
				_action(this, delta);
			
			UpdateAttachments(delta);
		}
		
		virtual void UpdateEditMode(float delta)
		{
			UpdateAttachmentsEditMode(delta);
		}
		
		RNAPI virtual bool CanUpdate(FrameID frame);
		
	protected:
		RNAPI virtual void WillUpdate(ChangeSet changeSet);
		RNAPI virtual void DidUpdate(ChangeSet changeSet);
		RNAPI void CleanUp() override;
		RNAPI void UpdateAttachments(float delta);
		RNAPI void UpdateAttachmentsEditMode(float delta);
		
		RNAPI virtual void ChildDidUpdate(SceneNode *child, ChangeSet changes) {}
		RNAPI virtual void ChildWillUpdate(SceneNode *child, ChangeSet changes) {}
		RNAPI virtual void WillAddChild(SceneNode *child) {}
		RNAPI virtual void DidAddChild(SceneNode *child)  {}
		RNAPI virtual void WillRemoveChild(SceneNode *child) {}
		RNAPI virtual void DidRemoveChild(SceneNode *child) {}
		
		RNAPI void UpdatedToFrame(FrameID frame) { _lastFrame.store(frame); }
		
		Observable<Vector3, SceneNode> _position;
		Observable<Vector3, SceneNode> _scale;
		Observable<Quaternion, SceneNode> _rotation;
		Vector3 _euler;
		
		AABB   _boundingBox;
		Sphere _boundingSphere;
		
		Signal<void (SceneNode *)> _cleanUpSignal;
		
	private:
		void Initialize();
		bool Compare(const SceneNode *other) const;
		void UpdateInternalData() const;
		void __BreakDependency(SceneNode *dependency);
		
		World *_world;
		bool _worldStatic;
		bool _worldInserted;
		
		mutable RecursiveSpinLock _parentChildLock;
		SceneNode *_parent;
		Array _children;
		
		mutable RecursiveSpinLock _attachmentsLock;
		Array _attachments;
		
		Priority _priority;
		std::atomic<uint32> _flags;
		std::atomic<FrameID> _lastFrame;
		
		uint8 renderGroup;
		uint8 collisionGroup;
		
		uint64 _uid;
		uint64 _lid;
		
		Observable<Tag, SceneNode> _tag;
		
		std::function<void (SceneNode *, float)> _action;
		std::string _debugName;
		
		RecursiveSpinLock _dependenciesLock;
		std::unordered_map<SceneNode *, Connection *> _dependencyMap;
		std::vector<SceneNode *> _dependencies;
		
		mutable RecursiveSpinLock _transformLock;
		mutable bool _updated;
		mutable Vector3 _worldPosition;
		mutable Quaternion _worldRotation;
		mutable Vector3 _worldScale;
		mutable Vector3 _worldEuler;
		
		mutable AABB _transformedBoundingBox;
		mutable Sphere _transformedBoundingSphere;
		
		mutable Matrix _worldTransform;
		mutable Matrix _localTransform;
		
		RNDeclareMeta(SceneNode)
	};
	
	
	
	RN_INLINE void SceneNode::Translate(const Vector3& trans)
	{
		_transformLock.Lock();
		WillUpdate(ChangeSet::Position);
		
		_position += trans;
		
		DidUpdate(ChangeSet::Position);
		_transformLock.Unlock();
	}
	
	RN_INLINE void SceneNode::Scale(const Vector3& scal)
	{
		_transformLock.Lock();
		WillUpdate(ChangeSet::Position);
		
		_scale += scal;
		
		DidUpdate(ChangeSet::Position);
		_transformLock.Unlock();
	}
	
	RN_INLINE void SceneNode::Rotate(const Vector3& rot)
	{
		_transformLock.Lock();
		
		WillUpdate(ChangeSet::Position);
		
		_euler += rot;
		_rotation = Quaternion(_euler);
		
		DidUpdate(ChangeSet::Position);
		
		_transformLock.Unlock();
	}
	
	RN_INLINE void SceneNode::Rotate(const Quaternion& rot)
	{
		_transformLock.Lock();
		
		WillUpdate(ChangeSet::Position);
		
		_rotation *= rot;
		
		DidUpdate(ChangeSet::Position);
		
		_transformLock.Unlock();
	}
	
	RN_INLINE void SceneNode::TranslateLocal(const Vector3& trans)
	{
		_transformLock.Lock();
		
		WillUpdate(ChangeSet::Position);
		_position += _rotation->GetRotatedVector(trans);
		DidUpdate(ChangeSet::Position);
		
		_transformLock.Unlock();
	}
	
	RN_INLINE void SceneNode::SetPosition(const Vector3& pos)
	{
		_transformLock.Lock();
		
		WillUpdate(ChangeSet::Position);
		_position = pos;
		DidUpdate(ChangeSet::Position);
		
		_transformLock.Unlock();
	}
	
	RN_INLINE void SceneNode::SetScale(const Vector3& scal)
	{
		_transformLock.Lock();
		
		WillUpdate(ChangeSet::Position);
		_scale = scal;
		DidUpdate(ChangeSet::Position);
		
		_transformLock.Unlock();
	}
	
	RN_INLINE void SceneNode::SetRotation(const Quaternion& rot)
	{
		_transformLock.Lock();
		
		WillUpdate(ChangeSet::Position);
		
		_euler    = rot.GetEulerAngle();
		_rotation = rot;
		
		DidUpdate(ChangeSet::Position);
		
		_transformLock.Unlock();
	}
	
	
	RN_INLINE void SceneNode::SetWorldPosition(const Vector3& pos)
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(_transformLock);
		
		std::lock(lock1, lock2);
		
		if(!_parent)
		{
			SetPosition(pos);
			
			lock1.unlock();
			lock2.unlock();
			return;
		}
		
		WillUpdate(ChangeSet::Position);
		
		Quaternion temp;
		temp = temp / _parent->GetWorldRotation();
		
		_position = temp.GetRotatedVector(pos) - temp.GetRotatedVector(GetWorldPosition());
		
		DidUpdate(ChangeSet::Position);
		
		lock1.unlock();
		lock2.unlock();
	}
	
	RN_INLINE void SceneNode::SetWorldScale(const Vector3& scal)
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(_transformLock);
		
		std::lock(lock1, lock2);
		
		if(!_parent)
		{
			SetScale(scal);
			
			lock1.unlock();
			lock2.unlock();
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
		
		lock1.unlock();
		lock2.unlock();
	}
	
	RN_INLINE void SceneNode::SetWorldRotation(const Quaternion& rot)
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(_transformLock);
		
		std::lock(lock1, lock2);
		
		if(!_parent)
		{
			SetRotation(rot);
			
			lock1.unlock();
			lock2.unlock();
			return;
		}
		
		WillUpdate(ChangeSet::Position);
		
		_rotation = rot / _parent->GetWorldRotation();
		_euler = _rotation->GetEulerAngle();
		
		DidUpdate(ChangeSet::Position);
		
		lock1.unlock();
		lock2.unlock();
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
	
	
	RN_INLINE Vector3 SceneNode::GetPosition() const
	{
		_transformLock.Lock();
		Vector3 result(_position);
		_transformLock.Unlock();
		
		return result;
	}
	RN_INLINE Vector3 SceneNode::GetScale() const
	{
		_transformLock.Lock();
		Vector3 result(_scale);
		_transformLock.Unlock();
		
		return result;
	}
	RN_INLINE Quaternion SceneNode::GetRotation() const
	{
		_transformLock.Lock();
		Quaternion result(_rotation);
		_transformLock.Unlock();
		
		return result;
	}
	RN_INLINE Vector3 SceneNode::GetEulerAngle() const
	{
		_transformLock.Lock();
		Vector3 result(_euler);
		_transformLock.Unlock();
		
		return result;
	}
	
	RN_INLINE Vector3 SceneNode::GetWorldPosition() const
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(_transformLock);
		
		std::lock(lock1, lock2);
		
		UpdateInternalData();
		Vector3 result(_worldPosition);
		
		lock1.unlock();
		lock2.unlock();
		
		return result;
	}
	RN_INLINE Vector3 SceneNode::GetWorldScale() const
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(_transformLock);
		
		std::lock(lock1, lock2);
		
		UpdateInternalData();
		Vector3 result(_worldScale);
		
		lock1.unlock();
		lock2.unlock();
		
		return result;
	}
	RN_INLINE Vector3 SceneNode::GetWorldEulerAngle() const
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(_transformLock);
		
		std::lock(lock1, lock2);
		
		UpdateInternalData();
		Vector3 result(_worldEuler);
		
		lock1.unlock();
		lock2.unlock();
		
		return result;
	}
	RN_INLINE Quaternion SceneNode::GetWorldRotation() const
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(_transformLock);
		
		std::lock(lock1, lock2);
		
		UpdateInternalData();
		Quaternion result(_worldRotation);
		
		lock1.unlock();
		lock2.unlock();
		
		return result;
	}
	
	RN_INLINE const Vector3 &SceneNode::GetWorldPosition_NoLock() const
	{
		UpdateInternalData();
		return _worldPosition;
	}
	RN_INLINE const Vector3 &SceneNode::GetWorldScale_NoLock() const
	{
		UpdateInternalData();
		return _worldScale;
	}
	RN_INLINE const Vector3 &SceneNode::GetWorldEulerAngle_NoLock() const
	{
		UpdateInternalData();
		return _worldEuler;
	}
	RN_INLINE const Quaternion &SceneNode::GetWorldRotation_NoLock() const
	{
		UpdateInternalData();
		return _worldRotation;
	}
	
	
	RN_INLINE Matrix SceneNode::GetTransform() const
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(_transformLock);
		
		std::lock(lock1, lock2);
		
		UpdateInternalData();
		Matrix result(_localTransform);
		
		lock1.unlock();
		lock2.unlock();
		
		return result;
	}
	
	RN_INLINE Matrix SceneNode::GetWorldTransform() const
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(_transformLock);
		
		std::lock(lock1, lock2);
		
		UpdateInternalData();
		Matrix result(_worldTransform);
		
		lock1.unlock();
		lock2.unlock();
		
		return result;
	}
	
	RN_INLINE AABB SceneNode::GetBoundingBox() const
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(_transformLock);
		
		std::lock(lock1, lock2);
		
		UpdateInternalData();
		AABB result(_transformedBoundingBox);
		
		lock1.unlock();
		lock2.unlock();
		
		return result;
	}
	
	RN_INLINE Sphere SceneNode::GetBoundingSphere() const
	{
		stl::lockable_shim<RecursiveSpinLock> lock1(_parentChildLock);
		stl::lockable_shim<RecursiveSpinLock> lock2(_transformLock);
		
		std::lock(lock1, lock2);
		
		UpdateInternalData();
		Sphere result(_transformedBoundingSphere);
		
		lock1.unlock();
		lock2.unlock();
		
		return result;
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
			
			_transformedBoundingBox = _boundingBox;
			
			_transformedBoundingBox.position = _worldPosition;
			_transformedBoundingBox *= _worldScale;
			_transformedBoundingBox.SetRotation(_worldRotation);
			
			_transformedBoundingSphere = _boundingSphere;
			_transformedBoundingSphere.position = _worldPosition;
			_transformedBoundingSphere *= _worldScale;
			_transformedBoundingSphere.SetRotation(_worldRotation);
			
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
