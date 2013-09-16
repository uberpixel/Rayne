//
//  RNMatrixQuaternion.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
	//column fucking major!!!!!111111
#if __RN_GENERATING_DOXYGEN
	class Matrix
#else
	class alignas(16) Matrix
#endif
	{
	public:
		RNAPI Matrix();

		RNAPI Matrix& operator*= (const Matrix& other);
		RNAPI Matrix operator* (const Matrix& other) const;
		RNAPI Vector3 operator* (const Vector3& other) const;
		RNAPI Vector4 operator* (const Vector4& other) const;

		RNAPI void MakeIdentity();
		RNAPI void MakeTranslate(const Vector3& other);
		RNAPI void MakeTranslate(const Vector4& other);
		RNAPI void MakeScale(const Vector3& other);
		RNAPI void MakeScale(const Vector4& other);
		RNAPI void MakeRotate(const Vector3& rot);
		RNAPI void MakeRotate(const Vector4& rot);
		RNAPI void MakeRotate(const Quaternion& rot);

		RNAPI void MakeProjectionOrthogonal(float left, float right, float bottom, float top, float clipnear, float clipfar);
		RNAPI void MakeProjectionPerspective(float arc, float aspect, float clipnear, float clipfar);
		RNAPI void MakeInverseProjectionPerspective(float arc, float aspect, float clipnear, float clipfar);

		RNAPI float GetDeterminant() const;
		RNAPI float GetDeterminantSubmatrix(const int k) const;
		
		RNAPI void SetTranslation(const Vector3& other);
		RNAPI void SetTranslation(const Vector4& other);
		RNAPI void SetScale(const Vector3& other);
		RNAPI void SetScale(const Vector4& other);

		RNAPI void Translate(const Vector3& trans);
		RNAPI void Translate(const Vector4& trans);
		RNAPI void Scale(const Vector3& scal);
		RNAPI void Scale(const Vector4& scal);
		RNAPI void Rotate(const Vector3& rot);
		RNAPI void Rotate(const Vector4& rot);
		RNAPI void Rotate(const Quaternion& rot);

		RNAPI void Transpose();

		RNAPI Vector3 Transform(const Vector3& other) const;
		RNAPI Vector4 Transform(const Vector4& other) const;

		RNAPI Matrix GetInverse() const;
	
#if RN_SIMD
		RN_INLINE void *operator new[](size_t size) { return Memory::AllocateSIMD(size); }
		RN_INLINE void operator delete[](void *ptr) { if(ptr) Memory::FreeSIMD(ptr); }
		
		union
		{
			float m[16];
			Vector4 vec[4];
		};
#else
		float m[16];
#endif
	};

	class Quaternion
	{
	public:
		RNAPI Quaternion();
		RNAPI Quaternion(float x, float y, float z, float w);
		RNAPI Quaternion(const Vector3& euler);
		RNAPI Quaternion(const Vector4& axis);

		RNAPI Quaternion& operator= (const Vector3& other);
		RNAPI Quaternion& operator= (const Vector4& other);

		RNAPI Quaternion& operator+= (const Quaternion& other);
		RNAPI Quaternion& operator-= (const Quaternion& other);
		RNAPI Quaternion& operator*= (const Quaternion& other);
		RNAPI Quaternion& operator/= (const Quaternion& other);

		RNAPI Quaternion& operator*= (const Vector4& other);
		RNAPI Quaternion& operator/= (const Vector4& other);

		RNAPI Quaternion& operator+= (const Vector3& other);
		RNAPI Quaternion& operator-= (const Vector3& other);

		RNAPI Quaternion& operator*= (float scalar);
		RNAPI Quaternion& operator/= (float scalar);

		RNAPI Quaternion operator+ (const Quaternion& other) const;
		RNAPI Quaternion operator- (const Quaternion& other) const;
		RNAPI Quaternion operator* (const Quaternion& other) const;
		RNAPI Quaternion operator/ (const Quaternion& other) const;

		RNAPI Quaternion operator+ (const Vector3& other) const;
		RNAPI Quaternion operator- (const Vector3& other) const;

		RNAPI Quaternion operator* (const Vector4& other) const;
		RNAPI Quaternion operator/ (const Vector4& other) const;

		RNAPI Quaternion operator* (float scalar) const;
		RNAPI Quaternion operator/ (float scalar) const;

		RNAPI void MakeIdentity();
		RNAPI void MakeEulerAngle(const Vector3& euler);
		RNAPI void MakeAxisAngle(const Vector4& euler);
		RNAPI void MakeLerpS(const Quaternion& start, const Quaternion& end, float factor);
		RNAPI void MakeLerpN(const Quaternion& start, const Quaternion& end, float factor);

		RNAPI void LookAt(const Vector3& dir, const Vector3& up=Vector3(0.0f, 1.0f, 0.0f), bool forceup=false);
		
		RNAPI Quaternion &Normalize();
		RNAPI Quaternion Normalize() const;
		RNAPI Quaternion &Conjugate();
		RNAPI Quaternion Conjugate() const;

		RNAPI Quaternion LerpS(const Quaternion& other, float factor) const;
		RNAPI Quaternion LerpN(const Quaternion& other, float factor) const;

		RNAPI Vector3 RotateVector(const Vector3& vec) const;
		RNAPI Vector4 RotateVector(const Vector4& vec) const;

		RNAPI Matrix GetRotationMatrix() const;

		RNAPI Vector3 GetEulerAngle() const;
		RNAPI Vector4 GetAxisAngle() const;

		RNAPI float Length() const;
		RNAPI float Dot(const Quaternion& other) const;

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
