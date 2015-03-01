//
//  RNMatrixQuaternion.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MATRIXQUATERNION_H__
#define __RAYNE_MATRIXQUATERNION_H__

#include "../Base/RNBase.h"
#include "RNVector.h"
#include "RNSIMD.h"

namespace RN
{
	class Quaternion;
	class alignas(16) Matrix
	{
	public:
		Matrix();
		
		bool operator== (const Matrix &other) const;
		bool operator!= (const Matrix &other) const;

		Matrix &operator*= (const Matrix &other);
		Matrix operator* (const Matrix &other) const;
		Vector3 operator* (const Vector3 &other) const;
		Vector4 operator* (const Vector4 &other) const;

		static Matrix WithIdentity();
		static Matrix WithTranslation(const Vector3 &translation);
		static Matrix WithTranslation(const Vector4 &translation);
		static Matrix WithScaling(const Vector3 &scaling);
		static Matrix WithScaling(const Vector4 &scaling);
		static Matrix WithRotation(const Vector3 &rotation);
		static Matrix WithRotation(const Vector4 &rotation);
		static Matrix WithRotation(const Quaternion &rotation);

		static Matrix WithProjectionOrthogonal(float left, float right, float bottom, float top, float clipnear, float clipfar);
		static Matrix WithProjectionPerspective(float arc, float aspect, float clipnear, float clipfar);
		static Matrix WithInverseProjectionPerspective(float arc, float aspect, float clipnear, float clipfar);

		float GetDeterminant() const;
		
		Vector3 GetEulerAngle() const;
		Vector4 GetAxisAngle() const;
		Quaternion GetQuaternion() const;

		void Translate(const Vector3 &translation);
		void Translate(const Vector4 &translation);
		void Scale(const Vector3 &scaling);
		void Scale(const Vector4 &scaling);
		void Rotate(const Vector3 &rotation);
		void Rotate(const Vector4 &rotation);
		void Rotate(const Quaternion &rotation);

		void Transpose();
		Matrix GetTransposed() const;
		void Inverse();
		Matrix GetInverse() const;
		
		bool IsEqual(const Matrix &other, float epsilon) const;
	
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
	private:
		float GetSubmatrixDeterminant(const int k) const;
		
	};

	class Quaternion
	{
	public:
		Quaternion();
		Quaternion(float x, float y, float z, float w);
		Quaternion(const Vector3 &euler);
		Quaternion(const Vector4 &axis);
		
		bool operator== (const Quaternion &other) const;
		bool operator!= (const Quaternion &other) const;

		Quaternion &operator+= (const Quaternion &other);
		Quaternion &operator-= (const Quaternion &other);
		Quaternion &operator*= (const Quaternion &other);
		Quaternion &operator/= (const Quaternion &other);
		Quaternion &operator*= (float scalar);
		Quaternion &operator/= (float scalar);

		Quaternion operator+ (const Quaternion &other) const;
		Quaternion operator- (const Quaternion &other) const;
		Quaternion operator* (const Quaternion &other) const;
		Quaternion operator/ (const Quaternion &other) const;
		Quaternion operator* (float scalar) const;
		Quaternion operator/ (float scalar) const;
		
		Quaternion &operator+= (const Vector3 &other);
		Quaternion &operator-= (const Vector3 &other);
		Quaternion operator+ (const Vector3 &other) const;
		Quaternion operator- (const Vector3 &other) const;

		static Quaternion WithIdentity();
		static Quaternion WithEulerAngle(const Vector3 &euler);
		static Quaternion WithAxisAngle(const Vector4 &euler);
		static Quaternion WithLerpSpherical(const Quaternion &start, const Quaternion &end, float factor);
		static Quaternion WithLerpLinear(const Quaternion &start, const Quaternion &end, float factor);
		static Quaternion WithLookAt(const Vector3 &dir, const Vector3 &up=Vector3(0.0f, 1.0f, 0.0f), bool forceup=false);
		
		Quaternion &Normalize();
		Quaternion GetNormalized() const;
		Quaternion &Conjugate();
		Quaternion GetConjugated() const;

		Quaternion GetLerpSpherical(const Quaternion &other, float factor) const;
		Quaternion GetLerpLinear(const Quaternion &other, float factor) const;

		Vector3 GetRotatedVector(const Vector3 &vec) const;
		Vector4 GetRotatedVector(const Vector4 &vec) const;

		Matrix GetRotationMatrix() const;

		Vector3 GetEulerAngle() const;
		Vector4 GetAxisAngle() const;

		float GetLength() const;
		float GetDotProduct(const Quaternion &other) const;
		
		bool IsEqual(const Quaternion &other, float epsilon) const;

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
