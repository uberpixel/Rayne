//
//  RNMatrixQuaternion.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MATRIXQUATERNION_H__
#define __RAYNE_MATRIXQUATERNION_H__

#include "RNBase.h"
#include "RNVector.h"

namespace RN
{
	class Quaternion;
	class Matrix
	{
	public:
		Matrix();
		Matrix(const Matrix& other);
		
		Matrix& operator*= (const Matrix& other);
		Matrix operator* (const Matrix& other) const;
		Vector3 operator* (const Vector3& other) const;
		Vector4 operator* (const Vector4& other) const;
		
		void MakeIdentity();
		void MakeTranslate(const Vector3& other);
		void MakeTranslate(const Vector4& other);
		void MakeScale(const Vector3& other);
		void MakeScale(const Vector4& other);
		void MakeRotate(const Vector3& rot);
		void MakeRotate(const Vector4& rot);
		void MakeRotate(const Quaternion& rot);
		
		void MakeProjectionOrthogonal(float left, float right, float bottom, float top, float clipnear, float clipfar);
		void MakeProjectionPerspective(float arc, float aspect, float clipnear, float clipfar);
		void MakeInverveProjectionPerspective(float arc, float aspect, float clipnear, float clipfar);
		
		float Determinant() const;
		float DeterminantSubmatrix(const int k) const;
		
		void Translate(const Vector3& trans);
		void Translate(const Vector4& trans);
		void Scale(const Vector3& scal);
		void Scale(const Vector4& scal);
		void Rotate(const Vector3& rot);
		void Rotate(const Vector4& rot);
		void Rotate(const Quaternion& rot);
		
		Vector3 Transform(const Vector3& other) const;
		Vector4 Transform(const Vector4& other) const;
		
		Matrix Inverse() const;
		
		struct
		{
			float m[16];
		};
	};
	
	class Quaternion
	{
	public:
		Quaternion();
		Quaternion(const Quaternion& other);
		Quaternion(const Vector3& euler);
		Quaternion(const Vector4& axis);
		
		void MakeIdentity();
		void MakeEulerAngle(const Vector3& euler);
		void MakeAxisAngle(const Vector4& euler);
		
		void LookAt(const Vector3& dir, const Vector3& up=Vector3(1.0f, 0.0f, 0.0f));
		void Normalize();
		
		Vector3 RotateEuler(const Vector3& vec) const;
		Vector4 RotateAxis(const Vector4& vec) const;
		
		Matrix RotationMatrix() const;
		
		Vector3 EulerAngle() const;
		Vector4 AxisAngle() const;
		float Length() const;
		
		struct
		{
			float x;
			float y;
			float z;
			float w;
		};
	};
}

#endif /* __RAYNE_MATRIXQUATERNION_H__ */
