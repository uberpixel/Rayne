//
//  RNSceneNode.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCENENODE_H__
#define __RAYNE_SCENENODE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"
#include "RNVector.h"
#include "RNArray.h"
#include "RNAABB.h"
#include "RNSphere.h"

namespace RN
{
	class Renderer;
	class Camera;
	class World;
	
	class SceneNode : public Object
	{
	friend class Renderer;
	friend class World;
	public:
		enum class Priority
		{
			UpdateEarly,
			UpdateDontCare,
			UpdateLate
		};
		
		RNAPI SceneNode();
		RNAPI SceneNode(const Vector3& position);
		RNAPI SceneNode(const Vector3& position, const Quaternion& rotation);
		RNAPI virtual ~SceneNode();
		
		RNAPI void Translate(const Vector3& trans);
		RNAPI void Scale(const Vector3& scal);
		RNAPI void Rotate(const Vector3& rot);
		
		RNAPI void TranslateLocal(const Vector3& trans);
		RNAPI void ScaleLocal(const Vector3& scal);
		
		RNAPI virtual void SetPosition(const Vector3& pos);
		RNAPI virtual void SetScale(const Vector3& scal);
		RNAPI virtual void SetRotation(const Quaternion& rot);
		
		RNAPI virtual void SetWorldPosition(const Vector3& pos);
		RNAPI virtual void SetWorldScale(const Vector3& scal);
		RNAPI virtual void SetWorldRotation(const Quaternion& rot);
		
		RNAPI void SetBoundingBox(const AABB& boundingBox, bool calculateBoundingSphere=true);
		RNAPI void SetBoundingSphere(const Sphere& boundingSphere);
		RNAPI void SetUpdatePriority(Priority priority);
		RNAPI void SetDebugName(const std::string& name);
		
		RNAPI virtual bool IsVisibleInCamera(Camera *camera);
		RNAPI virtual void Render(Renderer *renderer, Camera *camera);
		
		const Vector3& Position() const { return _position; }
		const Vector3& Scale() const { return _scale; }
		const Vector3& EulerAngle() const { return _euler; }
		const Quaternion& Rotation() const { return _rotation; }
		
		RNAPI const Vector3 Forward() const;
		RNAPI const Vector3 Up() const;
		RNAPI const Vector3 Right() const;
		
		RNAPI const Vector3& WorldPosition() const;
		RNAPI const Vector3& WorldScale() const;
		RNAPI const Vector3& WorldEulerAngle() const;
		RNAPI const Quaternion& WorldRotation() const;
		
		RNAPI const AABB& BoundingBox() const;
		RNAPI const Sphere& BoundingSphere() const;
		
		const std::string& DebugName() { return _debugName; }
		
		RNAPI void AttachChild(SceneNode *child);
		RNAPI void DetachChild(SceneNode *child);
		RNAPI void DetachAllChilds();
		RNAPI void DetachFromParent();
		
		RNAPI void SetAction(const std::function<void (SceneNode *, float)>& action);
		
		size_t Childs() const { return _childs.Count(); }
		SceneNode *Parent() const { return _parent; }
		FrameID LastFrame() const { return _lastFrame; }
		World *Container() const { return _world; }
		Priority UpdatePriority() const { return _priority; }
		
		template<typename T=SceneNode>
		T *ChildAtIndex(size_t index) const { return static_cast<T *>(_childs.ObjectAtIndex(index)); }
		
		RNAPI virtual class Hit CastRay(const Vector3 &position, const Vector3 &direction);
		
		RNAPI const Matrix& WorldTransform() const;
		RNAPI const Matrix& LocalTransform() const;
		
		virtual void Update(float delta)
		{
			if(_action)
				_action(this, delta);
		}
		
		virtual bool CanUpdate(FrameID frame)
		{
			if(_parent)
				return (_parent->_lastFrame >= frame);
			
			return true;
		}
		
		int8 renderGroup;
		int8 collisionGroup;
		
	protected:
		RNAPI void DidUpdate();
		RNAPI void UpdateInternalData() const;
		RNAPI void CleanUp() override;
		
		RNAPI virtual void ChildDidUpdate(SceneNode *child) {}
		RNAPI virtual void DidAddChild(SceneNode *child)  {}
		RNAPI virtual void WillRemoveChild(SceneNode *child) {}
		
		void UpdatedToFrame(FrameID frame) { _lastFrame = frame; }
		
		
		Vector3 _position;
		Vector3 _scale;
		Quaternion _rotation;
		Vector3 _euler;	//there has to be a way to fix this in the quaternion class somehow...
		
		AABB _boundingBox;
		Sphere _boundingSphere;
		
	private:
		void Initialize();
		
		World *_world;
		SceneNode *_parent;
		Array _childs;
		
		FrameID _lastFrame;
		Priority _priority;
		
		std::function<void (SceneNode *, float)> _action;
		std::string _debugName;
		
		mutable bool _updated;
		mutable SpinLock _updateLock;
		mutable Vector3 _worldPosition;
		mutable Quaternion _worldRotation;
		mutable Vector3 _worldScale;
		mutable Vector3 _worldEuler;
		
		mutable AABB _transformedBoundingBox;
		mutable Sphere _transformedBoundingSphere;
		
		mutable Matrix _worldTransform;
		mutable Matrix _localTransform;
		
		RNDefineMetaWithTraits(SceneNode, Object, MetaClassTraitCronstructable)
	};
	
	
	
	RN_INLINE void SceneNode::Translate(const Vector3& trans)
	{
		_position += trans;
		DidUpdate();
	}
	
	RN_INLINE void SceneNode::Scale(const Vector3& scal)
	{
		_scale += scal;
		DidUpdate();
	}
	
	RN_INLINE void SceneNode::Rotate(const Vector3& rot)
	{
		_euler += rot;
		_rotation = Quaternion(_euler);
		DidUpdate();
	}
	
	
	RN_INLINE void SceneNode::TranslateLocal(const Vector3& trans)
	{
		_position += _rotation.RotateVector(trans);
		DidUpdate();
	}
	
	RN_INLINE void SceneNode::ScaleLocal(const Vector3& scal)
	{
		_scale += _rotation.RotateVector(scal);
		DidUpdate();
	}
	
	
	RN_INLINE void SceneNode::SetPosition(const Vector3& pos)
	{
		_position = pos;
		DidUpdate();
	}
	
	RN_INLINE void SceneNode::SetScale(const Vector3& scal)
	{
		_scale = scal;
		DidUpdate();
	}
	
	RN_INLINE void SceneNode::SetRotation(const Quaternion& rot)
	{
		_euler = rot.EulerAngle();
		_rotation = rot;
		
		DidUpdate();
	}
	
	
	RN_INLINE void SceneNode::SetWorldPosition(const Vector3& pos)
	{
		if(!_parent)
		{
			SetPosition(pos);
			return;
		}
		
		Quaternion temp;
		temp = temp / _parent->WorldRotation();
		
		_position = temp.RotateVector(pos) - temp.RotateVector(WorldPosition());
		DidUpdate();
	}
	
	RN_INLINE void SceneNode::SetWorldScale(const Vector3& scal)
	{
		if(!_parent)
		{
			SetScale(scal);
			return;
		}
		
		_scale = scal - WorldScale();
		DidUpdate();
	}
	
	RN_INLINE void SceneNode::SetWorldRotation(const Quaternion& rot)
	{
		if(!_parent)
		{
			SetRotation(rot);
			return;
		}
		
		_rotation = rot / WorldRotation();
		_euler = _rotation.EulerAngle();
		
		DidUpdate();
	}
	
	RN_INLINE const Vector3 SceneNode::Forward() const
	{
		Vector3 forward = WorldRotation().RotateVector(Vector3(0.0, 0.0, 1.0));
		return forward;
	}
	
	RN_INLINE const Vector3 SceneNode::Up() const
	{
		Vector3 up = WorldRotation().RotateVector(Vector3(0.0, 1.0, 0.0));
		return up;
	}
	
	RN_INLINE const Vector3 SceneNode::Right() const
	{
		Vector3 right = WorldRotation().RotateVector(Vector3(1.0, 0.0, 0.0));
		return right;
	}
	
	
	RN_INLINE const Vector3& SceneNode::WorldPosition() const
	{
		UpdateInternalData();
		return _worldPosition;
	}
	RN_INLINE const Vector3& SceneNode::WorldScale() const
	{
		UpdateInternalData();
		return _worldScale;
	}
	RN_INLINE const Vector3& SceneNode::WorldEulerAngle() const
	{
		UpdateInternalData();
		return _worldEuler;
	}
	RN_INLINE const Quaternion& SceneNode::WorldRotation() const
	{
		UpdateInternalData();
		return _worldRotation;
	}
	

	
	RN_INLINE const Matrix& SceneNode::LocalTransform() const
	{
		UpdateInternalData();
		return _localTransform;
	}
	
	RN_INLINE const Matrix& SceneNode::WorldTransform() const
	{
		UpdateInternalData();
		return _worldTransform;
	}
	
	RN_INLINE const AABB& SceneNode::BoundingBox() const
	{
		UpdateInternalData();
		return _transformedBoundingBox;
	}
	
	RN_INLINE const Sphere& SceneNode::BoundingSphere() const
	{
		UpdateInternalData();
		return _transformedBoundingSphere;
	}
	
	
	RN_INLINE void SceneNode::UpdateInternalData() const
	{
		_updateLock.Lock();
		
		if(_updated)
		{
			_localTransform.MakeTranslate(_position);
			_localTransform.Rotate(_rotation);
			_localTransform.Scale(_scale);
			
			if(_parent)
			{
				_parent->UpdateInternalData();
				
				_worldPosition = _parent->_worldPosition + _parent->_worldRotation.RotateVector(_position);
				_worldRotation = _parent->_worldRotation * _rotation;
				_worldScale = _parent->_worldScale + _scale;
				_worldEuler = _parent->_worldEuler + _euler;
				
				_worldTransform = _parent->_localTransform * _localTransform;
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
			_transformedBoundingBox.Rotate(_worldRotation);
			
			_transformedBoundingSphere = _boundingSphere;
			_transformedBoundingSphere.position = _worldPosition;
			_transformedBoundingSphere *= _worldScale;
			_transformedBoundingSphere.Rotate(_worldRotation);
			
			_updated = false;
			_updateLock.Unlock();
			
			size_t count = _childs.Count();
			for(size_t i=0; i<count; i++)
			{
				SceneNode *child = _childs.ObjectAtIndex<SceneNode>(i);
				child->DidUpdate();
			}
			
			return;
		}
		
		_updateLock.Unlock();
	}
}

#endif /* __RAYNE_SCENENODE_H__ */
