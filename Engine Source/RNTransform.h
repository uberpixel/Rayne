//
//  RNTransform.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_TRANSFORM_H__
#define __RAYNE_TRANSFORM_H__

#include "RNBase.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"
#include "RNVector.h"
#include "RNArray.h"
#include "RNSynchronization.h"

namespace RN
{
	class RenderingPipeline;
	class Transform
	{
	friend class RenderingPipeline;
	public:
		typedef enum
		{
			TransformTypeEntity,
			TransformTypeCamera
		} TransformType;
		
		virtual ~Transform();
		
		TransformType Type() const { return _type; }
		
		virtual void Translate(const Vector3& trans);
		virtual void Scale(const Vector3& scal);
		virtual void Rotate(const Vector3& rot);
		
		virtual void SetPosition(const Vector3& pos);
		virtual void SetScale(const Vector3& scal);
		virtual void SetRotation(const Quaternion& rot);
		
		const Past<Vector3>& Position() const { return _position; }
		const Past<Vector3>& Scale() const { return _scale; }
		const Past<Vector3>& EulerAngle() const { return _euler; }
		const Past<Quaternion>& Rotation() const { return _rotation; }
		
		const Past<Vector3>& WorldPosition() const { return _worldPosition; }
		const Past<Vector3>& WorldScale() const { return _worldScale; }
		const Past<Vector3>& WorldEulerAngle() const { return _worldEuler; }
		const Past<Quaternion>& WorldRotation() const { return _worldRotation; }
		
		void AttachChild(Transform *child);
		void DetachChild(Transform *child);
		void DetachAllChilds();
		void DetachFromParent();
		
		machine_uint Childs() const { return _childs.Count(); }
		Transform *ChildAtIndex(machine_uint index) const { return _childs.ObjectAtIndex(index); }
		
		const Past<Matrix>& WorldTransform() { return _worldTransform; }
		const Past<Matrix>& LocalTransform() { return _localTransform; }
		
		virtual void Update(float delta)
		{
			machine_uint count = _childs.Count();
			for(machine_uint i=0; i<count; i++)
			{
				Transform *child = _childs.ObjectAtIndex(i);
				child->Update(delta);
			}
		}
		
		virtual void PostUpdate()
		{
			_position.SynchronizePast();
			_scale.SynchronizePast();
			_rotation.SynchronizePast();
			_euler.SynchronizePast();
			
			_worldPosition.SynchronizePast();
			_worldScale.SynchronizePast();
			_worldRotation.SynchronizePast();
			_worldEuler.SynchronizePast();
			
			_worldTransform.SynchronizePast();
			_localTransform.SynchronizePast();
			
			_pastWorldTransform = _worldTransform.AccessPast();
			
			machine_uint count = _childs.Count();
			for(machine_uint i=0; i<count; i++)
			{
				Transform *child = _childs.ObjectAtIndex(i);
				child->PostUpdate();
			}
		}
		
	protected:
		Transform(TransformType type);
		Transform(TransformType type, const Vector3& position);
		Transform(TransformType type, const Vector3& position, const Quaternion& rotation);
		
		void DidUpdate();
		
		Past<Vector3> _position;
		Past<Vector3> _scale;
		Past<Quaternion> _rotation;
		Past<Vector3> _euler;	//there has to be a way to fix this in the quaternion class somehow...
		
	private:
		const Matrix *PastWorldTransform() const { return &_pastWorldTransform; }
		
		Transform *_parent;
		Array<Transform *> _childs;
		
		TransformType _type;
		
		Past<Vector3> _worldPosition;
		Past<Quaternion> _worldRotation;
		Past<Vector3> _worldScale;
		Past<Vector3> _worldEuler;
		
		Past<Matrix> _worldTransform;
		Past<Matrix> _localTransform;
		
		Matrix _pastWorldTransform;
	};
	
	
	
	RN_INLINE void Transform::Translate(const Vector3& trans)
	{
		_position += trans;
		DidUpdate();
	}
	
	RN_INLINE void Transform::Scale(const Vector3& scal)
	{
		_scale += scal;
		DidUpdate();
	}
	
	RN_INLINE void Transform::Rotate(const Vector3& rot)
	{
		_euler += rot;
		_rotation = Quaternion(_euler);
		DidUpdate();
	}
	
	
	
	RN_INLINE void Transform::SetPosition(const Vector3& pos)
	{
		_position = pos;
		DidUpdate();
	}
	
	RN_INLINE void Transform::SetScale(const Vector3& scal)
	{
		_scale = scal;
		DidUpdate();
	}
	
	RN_INLINE void Transform::SetRotation(const Quaternion& rot)
	{
		_euler = rot.EulerAngle();
		_rotation = rot;
		
		DidUpdate();
	}
	
	
	RN_INLINE void Transform::DidUpdate()
	{
		_localTransform->MakeTranslate(_position);
		_localTransform->Rotate(_rotation);
		_localTransform->Scale(_scale);
		
		if(_parent)
		{
			_worldPosition = _parent->_worldPosition + _position;
			_worldRotation = _parent->_worldRotation + _rotation;
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
		
		machine_uint count = _childs.Count();
		for(machine_uint i=0; i<count; i++)
		{
			Transform *child = _childs.ObjectAtIndex(i);
			child->DidUpdate();
		}
	}
}

#endif /* __RAYNE_TRANSFORM_H__ */
