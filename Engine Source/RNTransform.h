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
#include "RNObject.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"
#include "RNVector.h"
#include "RNArray.h"

namespace RN
{
	class Renderer;
	class Camera;
	class Transform : public Object
	{
	friend class Renderer;
	public:
		Transform();
		Transform(const Vector3& position);
		Transform(const Vector3& position, const Quaternion& rotation);
		virtual ~Transform();
		
		void Translate(const Vector3& trans, bool local=false);
		void Scale(const Vector3& scal);
		void Rotate(const Vector3& rot);
		
		virtual void SetPosition(const Vector3& pos);
		virtual void SetScale(const Vector3& scal);
		virtual void SetRotation(const Quaternion& rot);
		
		virtual bool IsVisibleInCamera(Camera *camera) { return false; }
		
		const Vector3& Position() const { return _position; }
		const Vector3& Scale() const { return _scale; }
		const Vector3& EulerAngle() const { return _euler; }
		const Quaternion& Rotation() const { return _rotation; }
		
		const Vector3& WorldPosition();
		const Vector3& WorldScale();
		const Vector3& WorldEulerAngle();
		const Quaternion& WorldRotation();
		
		void AttachChild(Transform *child);
		void DetachChild(Transform *child);
		void DetachAllChilds();
		void DetachFromParent();
		
		machine_uint Childs() const { return _childs.Count(); }
		Transform *Parent() const { return _parent; }
		
		template<typename T=Transform>
		T *ChildAtIndex(machine_uint index) const { return static_cast<T *>(_childs.ObjectAtIndex(index)); }
		
		const Matrix& WorldTransform();
		const Matrix& LocalTransform();
		
		virtual void Update(float delta)
		{}
		
	protected:		
		void DidUpdate();
		void UpdateInternalData();
		
		Vector3 _position;
		Vector3 _scale;
		Quaternion _rotation;
		Vector3 _euler;	//there has to be a way to fix this in the quaternion class somehow...
		
	private:
		Transform *_parent;
		Array<Transform *> _childs;
		bool _updated;
		
		Vector3 _worldPosition;
		Quaternion _worldRotation;
		Vector3 _worldScale;
		Vector3 _worldEuler;
		
		Matrix _worldTransform;
		Matrix _localTransform;
		
		RNDefineMeta(Transform, Object)
	};
	
	
	
	RN_INLINE void Transform::Translate(const Vector3& trans, bool local)
	{
		if(local)
		{
			_position += _rotation.RotateVector(trans);
			
			DidUpdate();
			return;
		}
		
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
	
	
	RN_INLINE const Vector3& Transform::WorldPosition()
	{
		UpdateInternalData();
		return _worldPosition;
	}
	RN_INLINE const Vector3& Transform::WorldScale()
	{
		UpdateInternalData();
		return _worldScale;
	}
	RN_INLINE const Vector3& Transform::WorldEulerAngle()
	{
		UpdateInternalData();
		return _worldEuler;
	}
	RN_INLINE const Quaternion& Transform::WorldRotation()
	{
		UpdateInternalData();
		return _worldRotation;
	}
	
	
	RN_INLINE const Matrix& Transform::LocalTransform()
	{
		UpdateInternalData();
		return _localTransform;
	}
	
	RN_INLINE const Matrix& Transform::WorldTransform()
	{
		UpdateInternalData();
		return _worldTransform;
	}
	
	
	RN_INLINE void Transform::DidUpdate()
	{
		_updated = true;
	}
	
	RN_INLINE void Transform::UpdateInternalData()
	{
		if(_updated)
		{
			_localTransform.MakeTranslate(_position);
			_localTransform.Rotate(_rotation);
			_localTransform.Scale(_scale);
			
			if(_parent)
			{
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
			
			machine_uint count = _childs.Count();
			for(machine_uint i=0; i<count; i++)
			{
				Transform *child = _childs.ObjectAtIndex(i);
				child->DidUpdate();
			}
			
			_updated = false;
		}
	}
}

#endif /* __RAYNE_TRANSFORM_H__ */
