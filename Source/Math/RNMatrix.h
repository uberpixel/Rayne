//
//  RNMatrix.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MATRIX_H__
#define __RAYNE_MATRIX_H__

#include "../Base/RNBase.h"
#include "RNVector.h"
#include "RNMatrixQuaternion.h"

namespace RN
{
	RN_INLINE Matrix::Matrix()
	{
		std::fill(m, m + 16, 0.0f);
		m[0] = m[5] = m[10] = m[15] = 1.0f;
	}

	RN_INLINE Matrix &Matrix::operator*= (const Matrix &other)
	{
#if RN_SIMD
		RN_ALIGNAS(16) float tmp[16];
		SIMD::VecFloat right, result;
		
		for(int i=0; i<16; i+=4)
		{
			right = SIMD::Set(other.m[i]);
			result = SIMD::Mul(vec[0], right);
			
			for(int j=1; j<4; j++)
			{
				right = SIMD::Set(other.m[i + j]);
				result = SIMD::Add(SIMD::Mul(vec[j], right), result);
			}
			
			SIMD::Store(result, &tmp[i]);
		}
		
		std::copy(tmp, tmp + 16, m);
#else
		float tmp[16];
		
		tmp[ 0] = m[ 0] * other.m[ 0] + m[ 4] * other.m[ 1] + m[ 8] * other.m[ 2] + m[12] * other.m[ 3];
		tmp[ 1] = m[ 1] * other.m[ 0] + m[ 5] * other.m[ 1] + m[ 9] * other.m[ 2] + m[13] * other.m[ 3];
		tmp[ 2] = m[ 2] * other.m[ 0] + m[ 6] * other.m[ 1] + m[10] * other.m[ 2] + m[14] * other.m[ 3];
		tmp[ 3] = m[ 3] * other.m[ 0] + m[ 7] * other.m[ 1] + m[11] * other.m[ 2] + m[15] * other.m[ 3];
		
		tmp[ 4] = m[ 0] * other.m[ 4] + m[ 4] * other.m[ 5] + m[ 8] * other.m[ 6] + m[12] * other.m[ 7];
		tmp[ 5] = m[ 1] * other.m[ 4] + m[ 5] * other.m[ 5] + m[ 9] * other.m[ 6] + m[13] * other.m[ 7];
		tmp[ 6] = m[ 2] * other.m[ 4] + m[ 6] * other.m[ 5] + m[10] * other.m[ 6] + m[14] * other.m[ 7];
		tmp[ 7] = m[ 3] * other.m[ 4] + m[ 7] * other.m[ 5] + m[11] * other.m[ 6] + m[15] * other.m[ 7];
		
		tmp[ 8] = m[ 0] * other.m[ 8] + m[ 4] * other.m[ 9] + m[ 8] * other.m[10] + m[12] * other.m[11];
		tmp[ 9] = m[ 1] * other.m[ 8] + m[ 5] * other.m[ 9] + m[ 9] * other.m[10] + m[13] * other.m[11];
		tmp[10] = m[ 2] * other.m[ 8] + m[ 6] * other.m[ 9] + m[10] * other.m[10] + m[14] * other.m[11];
		tmp[11] = m[ 3] * other.m[ 8] + m[ 7] * other.m[ 9] + m[11] * other.m[10] + m[15] * other.m[11];
		
		tmp[12] = m[ 0] * other.m[12] + m[ 4] * other.m[13] + m[ 8] * other.m[14] + m[12] * other.m[15];
		tmp[13] = m[ 1] * other.m[12] + m[ 5] * other.m[13] + m[ 9] * other.m[14] + m[13] * other.m[15];
		tmp[14] = m[ 2] * other.m[12] + m[ 6] * other.m[13] + m[10] * other.m[14] + m[14] * other.m[15];
		tmp[15] = m[ 3] * other.m[12] + m[ 7] * other.m[13] + m[11] * other.m[14] + m[15] * other.m[15];
		
		std::copy(tmp, tmp + 16, m);
#endif
		
		return *this;
	}

	RN_INLINE Matrix Matrix::operator* (const Matrix &other) const
	{
		Matrix matrix;
		
#if RN_SIMD
		SIMD::VecFloat right, result;
		
		for(int i=0; i<16; i+=4)
		{
			right = SIMD::Set(other.m[i]);
			result = SIMD::Mul(vec[0], right);
			
			for(int j=1; j<4; j++)
			{
				right = SIMD::Set(other.m[i + j]);
				result = SIMD::Add(SIMD::Mul(vec[j], right), result);
			}
			
			matrix.vec[i/4] = result;
		}
#else
		matrix.m[ 0] = m[ 0] * other.m[ 0] + m[ 4] * other.m[ 1] + m[ 8] * other.m[ 2] + m[12] * other.m[ 3];
		matrix.m[ 1] = m[ 1] * other.m[ 0] + m[ 5] * other.m[ 1] + m[ 9] * other.m[ 2] + m[13] * other.m[ 3];
		matrix.m[ 2] = m[ 2] * other.m[ 0] + m[ 6] * other.m[ 1] + m[10] * other.m[ 2] + m[14] * other.m[ 3];
		matrix.m[ 3] = m[ 3] * other.m[ 0] + m[ 7] * other.m[ 1] + m[11] * other.m[ 2] + m[15] * other.m[ 3];
		
		matrix.m[ 4] = m[ 0] * other.m[ 4] + m[ 4] * other.m[ 5] + m[ 8] * other.m[ 6] + m[12] * other.m[ 7];
		matrix.m[ 5] = m[ 1] * other.m[ 4] + m[ 5] * other.m[ 5] + m[ 9] * other.m[ 6] + m[13] * other.m[ 7];
		matrix.m[ 6] = m[ 2] * other.m[ 4] + m[ 6] * other.m[ 5] + m[10] * other.m[ 6] + m[14] * other.m[ 7];
		matrix.m[ 7] = m[ 3] * other.m[ 4] + m[ 7] * other.m[ 5] + m[11] * other.m[ 6] + m[15] * other.m[ 7];
		
		matrix.m[ 8] = m[ 0] * other.m[ 8] + m[ 4] * other.m[ 9] + m[ 8] * other.m[10] + m[12] * other.m[11];
		matrix.m[ 9] = m[ 1] * other.m[ 8] + m[ 5] * other.m[ 9] + m[ 9] * other.m[10] + m[13] * other.m[11];
		matrix.m[10] = m[ 2] * other.m[ 8] + m[ 6] * other.m[ 9] + m[10] * other.m[10] + m[14] * other.m[11];
		matrix.m[11] = m[ 3] * other.m[ 8] + m[ 7] * other.m[ 9] + m[11] * other.m[10] + m[15] * other.m[11];
		
		matrix.m[12] = m[ 0] * other.m[12] + m[ 4] * other.m[13] + m[ 8] * other.m[14] + m[12] * other.m[15];
		matrix.m[13] = m[ 1] * other.m[12] + m[ 5] * other.m[13] + m[ 9] * other.m[14] + m[13] * other.m[15];
		matrix.m[14] = m[ 2] * other.m[12] + m[ 6] * other.m[13] + m[10] * other.m[14] + m[14] * other.m[15];
		matrix.m[15] = m[ 3] * other.m[12] + m[ 7] * other.m[13] + m[11] * other.m[14] + m[15] * other.m[15];
#endif
		
		return matrix;
	}


	RN_INLINE Vector3 Matrix::operator* (const Vector3 &other) const
	{
		Vector3 result;

		result.x = m[0] * other.x + m[4] * other.y + m[ 8] * other.z + m[12];
		result.y = m[1] * other.x + m[5] * other.y + m[ 9] * other.z + m[13];
		result.z = m[2] * other.x + m[6] * other.y + m[10] * other.z + m[14];

		return result;
	}

	RN_INLINE Vector4 Matrix::operator* (const Vector4 &other) const
	{
		Vector4 result;

#if RN_SIMD
		result.simd = SIMD::Mul(vec[0], SIMD::Set(other.x));
		result.simd = SIMD::Add(result.simd, SIMD::Mul(vec[1], SIMD::Set(other.y)));
		result.simd = SIMD::Add(result.simd, SIMD::Mul(vec[2], SIMD::Set(other.z)));
		result.simd = SIMD::Add(result.simd, SIMD::Mul(vec[3], SIMD::Set(other.w)));
#else
		result.x = m[0] * other.x + m[4] * other.y + m[ 8] * other.z + m[12] * other.w;
		result.y = m[1] * other.x + m[5] * other.y + m[ 9] * other.z + m[13] * other.w;
		result.z = m[2] * other.x + m[6] * other.y + m[10] * other.z + m[14] * other.w;
		result.w = m[3] * other.x + m[7] * other.y + m[11] * other.z + m[15] * other.w;
#endif

		return result;
	}
	
	RN_INLINE bool Matrix::operator== (const Matrix &other) const
	{
		return IsEqual(other, k::EpsilonFloat);
	}
	
	RN_INLINE bool Matrix::operator!= (const Matrix &other) const
	{
		return !IsEqual(other, k::EpsilonFloat);
	}
	
	RN_INLINE Matrix Matrix::GetInverse() const
	{
		Matrix result;
		
		float det = GetDeterminant();
		
		for(int i=0; i<16; i++)
		{
			result.m[i] = GetSubmatrixDeterminant(i) / det;
		}
		
		return result;
	}
	
	RN_INLINE void Matrix::Inverse()
	{
		*this = GetInverse();
	}

	RN_INLINE Matrix Matrix::WithIdentity()
	{
		Matrix mat;
		
		std::fill(mat.m, mat.m + 16, 0.0f);
		mat.m[0] = mat.m[5] = mat.m[10] = mat.m[15] = 1.0f;
		
		return mat;
	}
	
	RN_INLINE Matrix Matrix::WithTranslation(const Vector3 &translation)
	{
		Matrix mat;
		
		mat.m[12] = translation.x;
		mat.m[13] = translation.y;
		mat.m[14] = translation.z;
		
		return mat;
	}
	
	RN_INLINE Matrix Matrix::WithTranslation(const Vector4 &translation)
	{
		Matrix mat;
		
		mat.m[12] = translation.x;
		mat.m[13] = translation.y;
		mat.m[14] = translation.z;
		mat.m[15] = translation.w;
		
		return mat;
	}
	
	RN_INLINE Matrix Matrix::WithScaling(const Vector3 &scaling)
	{
		Matrix mat;
		
		mat.m[0] = scaling.x;
		mat.m[5] = scaling.y;
		mat.m[10] = scaling.z;
		
		return mat;
	}
	
	RN_INLINE Matrix Matrix::WithScaling(const Vector4 &scaling)
	{
		Matrix mat;
		
		mat.m[0] = scaling.x;
		mat.m[5] = scaling.y;
		mat.m[10] = scaling.z;
		mat.m[15] = scaling.w;
		
		return mat;
	}
	
	RN_INLINE Matrix Matrix::WithRotation(const Vector3 &rotation)
	{
		Quaternion quat(rotation);
		return quat.GetRotationMatrix();
	}
	
	RN_INLINE Matrix Matrix::WithRotation(const Vector4 &rotation)
	{
		Quaternion quat(rotation);
		return quat.GetRotationMatrix();
	}
	
	RN_INLINE Matrix Matrix::WithRotation(const Quaternion &rotation)
	{
		Quaternion quat(rotation);
		return quat.GetRotationMatrix();
	}

	
	RN_INLINE Matrix Matrix::WithProjectionOrthogonal(float left, float right, float bottom, float top, float clipnear, float clipfar)
	{
		Matrix mat;
		
		float rl = right - left;
		float tb = top - bottom;
		float fn = clipfar - clipnear;
		float tx = - (right + left) / (right - left);
		float ty = - (top + bottom) / (top - bottom);
		float tz = - (clipfar + clipnear) / (clipfar - clipnear);
		
		mat.m[0] = 2.0f / rl;
		mat.m[5] = 2.0f / tb;
		mat.m[10] = -2.0f / fn;
		
		mat.m[12] = tx;
		mat.m[13] = ty;
		mat.m[14] = tz;
		mat.m[15] = 1.0f;
		
		return mat;
	}
	
	RN_INLINE Matrix Matrix::WithProjectionPerspective(float arc, float aspect, float clipnear, float clipfar)
	{
		Matrix mat;
		
		float xFac, yFac;
		yFac = tanf(arc * k::Pi / 360.0f);
		xFac = yFac * aspect;
		
		mat.m[0] = 1.0f / xFac;
		mat.m[5] = 1.0f / yFac;
		mat.m[10] = -(clipfar + clipnear) / (clipfar - clipnear);
		mat.m[11] = -1.0f;
		mat.m[14] = -(2.0f * clipfar * clipnear) / (clipfar - clipnear);
		mat.m[15] = 0.0f;
		
		return mat;
	}
	
	RN_INLINE Matrix Matrix::WithInverseProjectionPerspective(float arc, float aspect, float clipnear, float clipfar)
	{
		Matrix mat;
		
		float xFac, yFac;
		yFac = tanf(arc * k::Pi / 360.0f);
		xFac = yFac * aspect;
		
		mat.m[0] = xFac;
		mat.m[5] = yFac;
		mat.m[10] = 0.0f;
		mat.m[11] = -(clipfar - clipnear) / (2.0f * clipfar * clipnear);
		mat.m[14] = -1.0f;
		mat.m[15] = (clipfar + clipnear) / (2.0f * clipfar * clipnear);
		
		return mat;
	}
	
	RN_INLINE float Matrix::GetDeterminant() const
	{
		float det = m[0] * (m[5]*m[10]*m[15] + m[6]*m[11]*m[13] + m[7]*m[9]*m[14] - m[7]*m[10]*m[13] - m[6]*m[9]*m[15] - m[5]*m[11]*m[14]);
		det -= m[1] * (m[4]*m[10]*m[15] + m[6]*m[11]*m[12] + m[7]*m[8]*m[14] - m[7]*m[10]*m[12] - m[6]*m[8]*m[15] - m[4]*m[11]*m[14]);
		det += m[2] * (m[4]*m[9]*m[15] + m[5]*m[11]*m[12] + m[7]*m[8]*m[13] - m[7]*m[9]*m[12] - m[5]*m[8]*m[15] - m[4]*m[11]*m[13]);
		det -= m[3] * (m[4]*m[9]*m[14] + m[5]*m[10]*m[12] + m[6]*m[8]*m[13] - m[6]*m[9]*m[12] - m[5]*m[8]*m[14] - m[4]*m[10]*m[13]);
		return det;
	}

	RN_INLINE float Matrix::GetSubmatrixDeterminant(const int k) const
	{
		int i = k%4; // i <=> j
		int j = k/4;
		int i1 = (i == 0);
		int i2 = i1+1+(i == 1);
		int i3 = i2+1+(i == 2);
		int j1 = (j == 0);
		int j2 = j1+1+(j == 1);
		int j3 = j2+1+(j == 2);
		i1 *= 4;
		i2 *= 4;
		i3 *= 4;

		float det = m[i1+j1]*m[i2+j2]*m[i3+j3] + m[i1+j2]*m[i2+j3]*m[i3+j1] + m[i1+j3]*m[i2+j1]*m[i3+j2] - m[i1+j3]*m[i2+j2]*m[i3+j1] - m[i1+j1]*m[i2+j3]*m[i3+j2] - m[i1+j2]*m[i2+j1]*m[i3+j3];
		det *= 1-2*((i+j)%2);

		return det;
	}
	
	RN_INLINE Vector3 Matrix::GetEulerAngle() const
	{
		Vector3 result;
		
		result.y = asinf(fmax(fmin(-m[9], 1.0f), -1.0f));
		float cy = cosf(result.y);
		if(Math::FastAbs(cy) > k::EpsilonFloat)
		{
			result.x = atan2f(m[8]/cy, m[10]/cy);
			result.z = atan2f(m[1]/cy, m[5]/cy);
		}
		else
		{
			result.z = 0.0f;
			if(result.y > 0.0f)
			{
				result.x = atan2f(m[4], m[0]);
			}
			else
			{
				result.x = atan2f(-m[4], -m[0]);
			}
		}
		
		result *= 180.0f / k::Pi;
		return result;
	}
	
	RN_INLINE Vector4 Matrix::GetAxisAngle() const
	{
		return GetQuaternion().GetAxisAngle();
	}
	
	RN_INLINE Quaternion Matrix::GetQuaternion() const
	{
		Quaternion result;
		
		float zz = 0.25f - m[0] * 0.25f + m[10] * 0.25f - m[5] * 0.25f;
		
		result.x = sqrt(0.5f - m[5] * 0.5f - zz);
		result.y = sqrt(0.5f - m[0] * 0.5f - zz);
		result.z = sqrt(zz);
		result.w = (-0.5f * m[9] + result.y * result.z) / result.x;
		
		return result;
	}
	
	RN_INLINE void Matrix::Translate(const Vector3 &translation)
	{
#if RN_SIMD
		SIMD::VecFloat result = SIMD::Mul(vec[0], SIMD::Set(translation.x));
		result = SIMD::Add(result, SIMD::Mul(vec[1], SIMD::Set(translation.y)));
		result = SIMD::Add(result, SIMD::Mul(vec[2], SIMD::Set(translation.z)));
		
		vec[3] = SIMD::Add(result, vec[3]);
#else
		float tmp[4];
		
		tmp[0] = m[ 0] * translation.x + m[ 4] * translation.y + m[ 8] * translation.z + m[12];
		tmp[1] = m[ 1] * translation.x + m[ 5] * translation.y + m[ 9] * translation.z + m[13];
		tmp[2] = m[ 2] * translation.x + m[ 6] * translation.y + m[10] * translation.z + m[14];
		tmp[3] = m[ 3] * translation.x + m[ 7] * translation.y + m[11] * translation.z + m[15];
		
		m[12] = tmp[0];
		m[13] = tmp[1];
		m[14] = tmp[2];
		m[15] = tmp[3];
#endif
	}
	
	RN_INLINE void Matrix::Translate(const Vector4 &translation)
	{
#if RN_SIMD
		SIMD::VecFloat result = SIMD::Mul(vec[0], SIMD::Set(translation.x));
		result = SIMD::Add(result, SIMD::Mul(vec[1], SIMD::Set(translation.y)));
		result = SIMD::Add(result, SIMD::Mul(vec[2], SIMD::Set(translation.z)));
		
		vec[3] = SIMD::Add(result, SIMD::Mul(vec[3], SIMD::Set(translation.w)));
#else
		float tmp[4];
		
		tmp[0] = m[ 0] * translation.x + m[ 4] * translation.y + m[ 8] * translation.z + m[12] * translation.w;
		tmp[1] = m[ 1] * translation.x + m[ 5] * translation.y + m[ 9] * translation.z + m[13] * translation.w;
		tmp[2] = m[ 2] * translation.x + m[ 6] * translation.y + m[10] * translation.z + m[14] * translation.w;
		tmp[3] = m[ 3] * translation.x + m[ 7] * translation.y + m[11] * translation.z + m[15] * translation.w;
		
		m[12] = tmp[0];
		m[13] = tmp[1];
		m[14] = tmp[2];
		m[15] = tmp[3];
#endif
	}
	
	RN_INLINE void Matrix::Scale(const Vector3 &scaling)
	{
		*this *= Matrix::WithScaling(scaling);
	}
	
	RN_INLINE void Matrix::Scale(const Vector4 &scaling)
	{
		*this *= Matrix::WithScaling(scaling);
	}
	
	RN_INLINE void Matrix::Rotate(const Vector3 &rotation)
	{
		*this *= Matrix::WithRotation(rotation);
	}
	
	RN_INLINE void Matrix::Rotate(const Vector4 &rotation)
	{
		*this *= Matrix::WithRotation(rotation);
	}
	
	RN_INLINE void Matrix::Rotate(const Quaternion &rotation)
	{
		*this *= Matrix::WithRotation(rotation);
	}
	
	RN_INLINE void Matrix::Transpose()
	{
		float temp[16];
		
		temp[0] = m[0];
		temp[1] = m[4];
		temp[2] = m[8];
		temp[3] = m[12];
		
		temp[4] = m[1];
		temp[5] = m[5];
		temp[6] = m[9];
		temp[7] = m[13];
		
		temp[8] = m[2];
		temp[9] = m[6];
		temp[10] = m[10];
		temp[11] = m[14];
		
		temp[12] = m[3];
		temp[13] = m[7];
		temp[14] = m[11];
		temp[15] = m[15];
		
		std::copy(temp, temp + 16, m);
	}
	
	RN_INLINE Matrix Matrix::GetTransposed() const
	{
		Matrix result;
		
		result.m[0] = m[0];
		result.m[1] = m[4];
		result.m[2] = m[8];
		result.m[3] = m[12];
		
		result.m[4] = m[1];
		result.m[5] = m[5];
		result.m[6] = m[9];
		result.m[7] = m[13];
		
		result.m[8] = m[2];
		result.m[9] = m[6];
		result.m[10] = m[10];
		result.m[11] = m[14];
		
		result.m[12] = m[3];
		result.m[13] = m[7];
		result.m[14] = m[11];
		result.m[15] = m[15];
		
		return result;
	}
	
	RN_INLINE bool Matrix::IsEqual(const Matrix &other, float epsilon) const
	{
		if(Math::FastAbs(m[0] - other.m[0]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[1] - other.m[1]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[2] - other.m[2]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[3] - other.m[3]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[4] - other.m[4]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[5] - other.m[5]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[6] - other.m[6]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[7] - other.m[7]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[8] - other.m[8]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[9] - other.m[9]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[10] - other.m[10]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[11] - other.m[11]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[12] - other.m[12]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[13] - other.m[13]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[14] - other.m[14]) > epsilon)
			return false;
		
		if(Math::FastAbs(m[15] - other.m[15]) > epsilon)
			return false;
		
		return true;
	}
}

#endif /* __RAYNE_MATRIX_H__ */
