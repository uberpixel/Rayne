//
//  RNMatrixQuaternion.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
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

		Matrix& operator= (const Matrix& other);
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
		void MakeInverseProjectionPerspective(float arc, float aspect, float clipnear, float clipfar);

		float Determinant() const;
		float DeterminantSubmatrix(const int k) const;

		void Translate(const Vector3& trans);
		void Translate(const Vector4& trans);
		void Scale(const Vector3& scal);
		void Scale(const Vector4& scal);
		void Rotate(const Vector3& rot);
		void Rotate(const Vector4& rot);
		void Rotate(const Quaternion& rot);

		void Transpose();

		Vector3 Transform(const Vector3& other) const;
		Vector4 Transform(const Vector4& other) const;

		Matrix Inverse() const;

		float m[16];
	};

	class Quaternion
	{
	public:
		Quaternion();
		Quaternion(float x, float y, float z, float w);
		Quaternion(const Vector3& euler);
		Quaternion(const Vector4& axis);

		//Quaternion& operator= (const Quaternion& other);
		Quaternion& operator= (const Vector3& other);
		Quaternion& operator= (const Vector4& other);

		Quaternion& operator+= (const Quaternion& other);
		Quaternion& operator-= (const Quaternion& other);
		Quaternion& operator*= (const Quaternion& other);
		Quaternion& operator/= (const Quaternion& other);

		Quaternion& operator*= (const Vector4& other);
		Quaternion& operator/= (const Vector4& other);

		Quaternion& operator+= (const Vector3& other);
		Quaternion& operator-= (const Vector3& other);

		Quaternion& operator*= (float scalar);
		Quaternion& operator/= (float scalar);

		Quaternion operator+ (const Quaternion& other) const;
		Quaternion operator- (const Quaternion& other) const;
		Quaternion operator* (const Quaternion& other) const;
		Quaternion operator/ (const Quaternion& other) const;

		Quaternion operator+ (const Vector3& other) const;
		Quaternion operator- (const Vector3& other) const;

		Quaternion operator* (const Vector4& other) const;
		Quaternion operator/ (const Vector4& other) const;

		Quaternion operator* (float scalar) const;
		Quaternion operator/ (float scalar) const;

		void MakeIdentity();
		void MakeEulerAngle(const Vector3& euler);
		void MakeAxisAngle(const Vector4& euler);
		void MakeLerpS(const Quaternion& start, const Quaternion& end, float factor);
		void MakeLerpN(const Quaternion& start, const Quaternion& end, float factor);

		void LookAt(const Vector3& dir, const Vector3& up=Vector3(1.0f, 0.0f, 0.0f));
		void Normalize();
		void Conjugate();

		Quaternion LerpS(const Quaternion& other, float factor) const;
		Quaternion LerpN(const Quaternion& other, float factor) const;

		Vector3 RotateVector(const Vector3& vec) const;
		Vector4 RotateVector(const Vector4& vec) const;

		Matrix RotationMatrix() const;

		Vector3 EulerAngle() const;
		Vector4 AxisAngle() const;

		float Length() const;
		float Dot(const Quaternion& other) const;

		struct
		{
			float x;
			float y;
			float z;
			float w;
		};
	};

	#ifndef __GNUG__
	static_assert(std::is_trivially_copyable<Matrix>::value, "Matrix must be trivially copyable");
	static_assert(std::is_trivially_copyable<Quaternion>::value, "Quaternion must be trivially copyable");
	#endif
}

#endif /* __RAYNE_MATRIXQUATERNION_H__ */
