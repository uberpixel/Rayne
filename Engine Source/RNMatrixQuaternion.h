//
//  RNMatrixQuaternion.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MATRIXQUATERNION_H__
#define __RAYNE_MATRIXQUATERNION_H__

#include "RNBase.h"
#include "RNVector.h"
#include "RNSIMD.h"

namespace RN
{
	class Quaternion;
	class alignas(16) Matrix
	{
	public:
		Matrix();

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

		float GetDeterminant() const;
		float GetDeterminantSubmatrix(const int k) const;
		
		Vector3 GetEulerAngle() const;
		Quaternion GetQuaternion() const;
		
		void SetTranslation(const Vector3& other);
		void SetTranslation(const Vector4& other);
		void SetScale(const Vector3& other);
		void SetScale(const Vector4& other);

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

		Matrix GetInverse() const;
	
#if RN_SIMD
		RN_INLINE void *operator new[](size_t size) { return Memory::AllocateSIMD(size); }
		RN_INLINE void operator delete[](void *ptr) { if(ptr) Memory::FreeSIMD(ptr); }
		
		union
		{
			float m[16];
			SIMD::VecFloat vec[4];
		};
#else
		float m[16];
#endif
	};

	class Quaternion
	{
	public:
		Quaternion();
		Quaternion(float x, float y, float z, float w);
		Quaternion(const Vector3& euler);
		Quaternion(const Vector4& axis);

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

		void LookAt(const Vector3& dir, const Vector3& up=Vector3(0.0f, 1.0f, 0.0f), bool forceup=false);
		
		Quaternion &Normalize();
		Quaternion Normalize() const;
		Quaternion &Conjugate();
		Quaternion Conjugate() const;

		Quaternion LerpS(const Quaternion& other, float factor) const;
		Quaternion LerpN(const Quaternion& other, float factor) const;

		Vector3 RotateVector(const Vector3& vec) const;
		Vector4 RotateVector(const Vector4& vec) const;

		Matrix GetRotationMatrix() const;

		Vector3 GetEulerAngle() const;
		Vector4 GetAxisAngle() const;

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

#if !(RN_PLATFORM_LINUX)
	static_assert(std::is_trivially_copyable<Matrix>::value, "Matrix must be trivially copyable");
	static_assert(std::is_trivially_copyable<Quaternion>::value, "Quaternion must be trivially copyable");
#endif
}

#endif /* __RAYNE_MATRIXQUATERNION_H__ */
