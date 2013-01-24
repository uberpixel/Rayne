//
//  RNTransform.h
//  Rayne
//
//  Created by Sidney Just on 20.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#ifndef __RAYNE_TRANSFORM_H__
#define __RAYNE_TRANSFORM_H__

#include "RNBase.h"
#include "RNMatrixQuaternion.h"
#include "RNVector.h"

namespace RN
{
	class Transform
	{
	public:
		Transform();
		Transform(const Vector3& position);
		Transform(const Vector3& position, const Quaternion& rotation);
		
		void Translate(const Vector3& trans);
		void Scale(const Vector3& scal);
		void Rotate(const Vector3& rot);
		
		void SetPosition(const Vector3& pos);
		void SetScale(const Vector3& scal);
		void SetRotation(const Quaternion& rot);
		
		const Vector3& Position() const { return _position; }
		const Vector3& Scale() const { return _scale; }
		const Quaternion& Rotation() const { return _rotation; }
		
		const Matrix& Matrix();
		
	private:
		bool _didChange;
		
		Vector3 _position;
		Vector3 _scale;
		Quaternion _rotation;
		
		class Matrix _transform;
	};
	
	RN_INLINE Transform::Transform() :
		_scale(Vector3(1.0f))
	{
		_didChange = true;
	}
	
	RN_INLINE Transform::Transform(const Vector3& position) :
		_position(position),
		_scale(Vector3(1.0f))
	{
		_didChange = true;
	}
	
	RN_INLINE Transform::Transform(const Vector3& position, const Quaternion& rotation) :
		_position(position),
		_scale(Vector3(1.0f)),
		_rotation(rotation)
	{
		_didChange = true;
	}
	
	RN_INLINE void Transform::Translate(const Vector3& trans)
	{
		_position += trans;
		_didChange = true;
	}
	
	RN_INLINE void Transform::Scale(const Vector3& scal)
	{
		_scale += scal;
		_didChange = true;
	}
	
	RN_INLINE void Transform::Rotate(const Vector3& rot)
	{
		_rotation += rot;
		_didChange = true;
	}
	
	RN_INLINE void Transform::SetPosition(const Vector3& pos)
	{
		_position = pos;
		_didChange = true;
	}
	
	RN_INLINE void Transform::SetScale(const Vector3& scal)
	{
		_scale = scal;
		_didChange = true;
	}
	
	RN_INLINE void Transform::SetRotation(const Quaternion& rot)
	{
		_rotation = rot;
		_didChange = true;
	}
	
	RN_INLINE const Matrix& Transform::Matrix()
	{
		if(_didChange)
		{
			_transform.MakeTranslate(_position);
			_transform.Scale(_scale);
			_transform.Rotate(_rotation);
			
			_didChange = false;
		}
		
		return _transform;
	}
}

#endif /* __RAYNE_TRANSFORM_H__ */
