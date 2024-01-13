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

/*
#if RN_PLATFORM_INTEL
#include <immintrin.h>
#include <smmintrin.h>
#elif RN_PLATFORM_ARM
#include "../Vendor/sse2neon/sse2neon.h"
#endif
*/
namespace RN
{
	RN_INLINE Matrix::Matrix()
	{
		std::fill(m, m + 16, 0.0f);
		m[0] = m[5] = m[10] = m[15] = 1.0f;
	}

	RN_INLINE Matrix &Matrix::operator*= (const Matrix &other)
	{
/*		float temp[16];
		
		__m128 simdResult;
		simdResult = _mm_mul_ps(_mm_loadu_ps(&m[0]), _mm_load1_ps(&other.m[0]));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[4]), _mm_load1_ps(&other.m[1])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[8]), _mm_load1_ps(&other.m[2])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[12]), _mm_load1_ps(&other.m[3])));
		_mm_storeu_ps(&temp[0], simdResult);
		
		simdResult = _mm_mul_ps(_mm_loadu_ps(&m[0]), _mm_load1_ps(&other.m[4]));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[4]), _mm_load1_ps(&other.m[5])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[8]), _mm_load1_ps(&other.m[6])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[12]), _mm_load1_ps(&other.m[7])));
		_mm_storeu_ps(&temp[4], simdResult);
		
		simdResult = _mm_mul_ps(_mm_loadu_ps(&m[0]), _mm_load1_ps(&other.m[8]));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[4]), _mm_load1_ps(&other.m[9])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[8]), _mm_load1_ps(&other.m[10])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[12]), _mm_load1_ps(&other.m[11])));
		_mm_storeu_ps(&temp[8], simdResult);
		
		simdResult = _mm_mul_ps(_mm_loadu_ps(&m[0]), _mm_load1_ps(&other.m[12]));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[4]), _mm_load1_ps(&other.m[13])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[8]), _mm_load1_ps(&other.m[14])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[12]), _mm_load1_ps(&other.m[15])));
		_mm_storeu_ps(&temp[12], simdResult);
		
		std::copy(temp, temp + 16, m);*/

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
		
		return *this;
	}

	RN_INLINE Matrix Matrix::operator* (const Matrix &other) const
	{
		Matrix matrix;
		
/*		__m128 simdResult;
		
		simdResult = _mm_mul_ps(_mm_loadu_ps(&m[0]), _mm_load1_ps(&other.m[0]));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[4]), _mm_load1_ps(&other.m[1])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[8]), _mm_load1_ps(&other.m[2])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[12]), _mm_load1_ps(&other.m[3])));
		_mm_storeu_ps(&matrix.m[0], simdResult);
		
		simdResult = _mm_mul_ps(_mm_loadu_ps(&m[0]), _mm_load1_ps(&other.m[4]));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[4]), _mm_load1_ps(&other.m[5])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[8]), _mm_load1_ps(&other.m[6])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[12]), _mm_load1_ps(&other.m[7])));
		_mm_storeu_ps(&matrix.m[4], simdResult);
		
		simdResult = _mm_mul_ps(_mm_loadu_ps(&m[0]), _mm_load1_ps(&other.m[8]));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[4]), _mm_load1_ps(&other.m[9])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[8]), _mm_load1_ps(&other.m[10])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[12]), _mm_load1_ps(&other.m[11])));
		_mm_storeu_ps(&matrix.m[8], simdResult);
		
		simdResult = _mm_mul_ps(_mm_loadu_ps(&m[0]), _mm_load1_ps(&other.m[12]));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[4]), _mm_load1_ps(&other.m[13])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[8]), _mm_load1_ps(&other.m[14])));
		simdResult = _mm_add_ps(simdResult, _mm_mul_ps(_mm_loadu_ps(&m[12]), _mm_load1_ps(&other.m[15])));
		_mm_storeu_ps(&matrix.m[12], simdResult);*/
		

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
		
		return matrix;
	}


	RN_INLINE Vector3 Matrix::operator* (const Vector3 &other) const
	{
		Vector3 result;

		result.x = m[0] * other.x + m[4] * other.y + m[ 8] * other.z + m[12];
		result.y = m[1] * other.x + m[5] * other.y + m[ 9] * other.z + m[13];
		result.z = m[2] * other.x + m[6] * other.y + m[10] * other.z + m[14];
		
		
		//Slower than the above...
		/*__m128 simdVector0 = _mm_load_ps1(&other.x);
		__m128 simdMatrixColumn0 = _mm_loadu_ps(&m[0]);
		__m128 r0 = _mm_mul_ps(simdVector0, simdMatrixColumn0);
		
		__m128 simdVector1 = _mm_load_ps1(&other.y);
		__m128 simdMatrixColumn1 = _mm_loadu_ps(&m[4]);
		__m128 r1 = _mm_mul_ps(simdVector1, simdMatrixColumn1);
		
		__m128 simdVector2 = _mm_load_ps1(&other.z);
		__m128 simdMatrixColumn2 = _mm_loadu_ps(&m[8]);
		__m128 r2 = _mm_mul_ps(simdVector2, simdMatrixColumn2);
		
		__m128 r3 = _mm_loadu_ps(&m[12]);
		
		r0 = _mm_add_ps(r0, r1);
		r2 = _mm_add_ps(r2, r3);
		r0 = _mm_add_ps(r0, r2);
		
		float temp[4];
		_mm_storeu_ps(temp, r0);
		result.x = temp[0];
		result.y = temp[1];
		result.z = temp[2];*/

		return result;
	}

	RN_INLINE Vector4 Matrix::operator* (const Vector4 &other) const
	{
		Vector4 result;
		
/*		__m128 simdVector = _mm_loadu_ps(&other.x);
		
		float temp[4] = {m[0], m[4], m[8], m[12]};
		__m128 simdMatrix0 = _mm_loadu_ps(temp);
		temp[0] = m[1]; temp[1] = m[5]; temp[2] = m[9]; temp[3] = m[13];
		__m128 simdMatrix1 = _mm_loadu_ps(temp);
		temp[0] = m[2]; temp[1] = m[6]; temp[2] = m[10]; temp[3] = m[14];
		__m128 simdMatrix2 = _mm_loadu_ps(temp);
		temp[0] = m[3]; temp[1] = m[7]; temp[2] = m[11]; temp[3] = m[15];
		__m128 simdMatrix3 = _mm_loadu_ps(temp);
		
		__m128 r0 = _mm_dp_ps(simdVector, simdMatrix0, 0xff);
		__m128 r1 = _mm_dp_ps(simdVector, simdMatrix1, 0xff);
		__m128 r2 = _mm_dp_ps(simdVector, simdMatrix2, 0xff);
		__m128 r3 = _mm_dp_ps(simdVector, simdMatrix3, 0xff);
		
		_mm_store_ss(&result.x, r0);
		_mm_store_ss(&result.y, r1);
		_mm_store_ss(&result.z, r2);
		_mm_store_ss(&result.w, r3);*/
		
		/*__m128 simdVector0 = _mm_load_ps1(&other.x);
		__m128 simdMatrixColumn0 = _mm_loadu_ps(&m[0]);
		__m128 r0 = _mm_mul_ps(simdVector0, simdMatrixColumn0);
		
		__m128 simdVector1 = _mm_load_ps1(&other.y);
		__m128 simdMatrixColumn1 = _mm_loadu_ps(&m[4]);
		__m128 r1 = _mm_mul_ps(simdVector1, simdMatrixColumn1);
		
		__m128 simdVector2 = _mm_load_ps1(&other.z);
		__m128 simdMatrixColumn2 = _mm_loadu_ps(&m[8]);
		__m128 r2 = _mm_mul_ps(simdVector2, simdMatrixColumn2);
		
		__m128 r3 = _mm_loadu_ps(&m[12]);
		
		r0 = _mm_add_ps(r0, r1);
		r2 = _mm_add_ps(r2, r3);
		r0 = _mm_add_ps(r0, r2);
		
		_mm_storeu_ps(&result.x, r0);*/

		result.x = m[0] * other.x + m[4] * other.y + m[ 8] * other.z + m[12] * other.w;
		result.y = m[1] * other.x + m[5] * other.y + m[ 9] * other.z + m[13] * other.w;
		result.z = m[2] * other.x + m[6] * other.y + m[10] * other.z + m[14] * other.w;
		result.w = m[3] * other.x + m[7] * other.y + m[11] * other.z + m[15] * other.w;

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
		float tz = - (clipnear) / (clipfar - clipnear);
		
		mat.m[0] = 2.0f / rl;
		mat.m[5] = 2.0f / tb;
		mat.m[10] = 1.0f / fn;
		
		mat.m[12] = tx;
		mat.m[13] = ty;
		mat.m[14] = -tz + 1.0;
		mat.m[15] = 1.0f;
		
		//Using these will put the near plane at 0 and far plane at 1, while the above does the opposite for better precision with floating point depth buffer
		//mat.m[10] = -1.0f / fn;
		//mat.m[14] = tz;
		
		return mat;
	}
	
	RN_INLINE Matrix Matrix::WithProjectionPerspective(float arc, float aspect, float clipnear, float clipfar)
	{
		Matrix mat;
		
		float rad = arc * k::Pi / 180.0f;
		float h = std::cos(0.5f * rad) / std::sin(0.5f * rad);
		float w = h / aspect;
		
		mat.m[0] = w;
		mat.m[5] = h;
		mat.m[10] = clipfar/(clipfar - clipnear) - 1.0;
		mat.m[11] = -1.0f;
		mat.m[14] = (clipfar * clipnear) / (clipfar - clipnear);
		mat.m[15] = 0.0f;
		
		//Using these will put the near plane at 0 and far plane at 1, while the above does the opposite for better precision with floating point depth buffer
		//mat.m[10] = -clipfar / (clipfar - clipnear);
		//mat.m[14] = -(clipfar * clipnear) / (clipfar - clipnear);
		
		return mat;
	}

	RN_INLINE Matrix Matrix::WithProjectionPerspective(float leftTangent, float rightTangent, float topTangent, float bottomTangent, float clipnear, float clipfar)
	{
		Matrix mat;
		
		leftTangent = -leftTangent * clipnear;
		rightTangent = rightTangent * clipnear;
		topTangent = topTangent * clipnear;
		bottomTangent = -bottomTangent * clipnear;
		
		mat.m[0] = 2.0f * clipnear / (rightTangent - leftTangent);
		mat.m[2] = (rightTangent + leftTangent) / (rightTangent - leftTangent);
		mat.m[5] = 2.0f * clipnear / (topTangent - bottomTangent);
		mat.m[6] = (topTangent + bottomTangent) / (topTangent - bottomTangent);
		mat.m[10] = clipfar/(clipfar - clipnear) - 1.0;
		mat.m[11] = -1.0f;
		mat.m[14] = (clipfar * clipnear) / (clipfar - clipnear);
		mat.m[15] = 0.0f;
		
		if(isinf(clipfar))
		{
			mat.m[10] = 0;
			mat.m[14] = clipnear;
		}
		
		//Using these will put the near plane at 0 and far plane at 1, while the above does the opposite for better precision with floating point depth buffer
		//mat.m[10] = -clipfar / (clipfar - clipnear);
		//mat.m[14] = -(clipfar * clipnear) / (clipfar - clipnear);
		
		return mat;
	}
	
	//TODO: Fix...
/*	RN_INLINE Matrix Matrix::WithInverseProjectionPerspective(float arc, float aspect, float clipnear, float clipfar)
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
	}*/
	
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
		float tmp[4];
		
		tmp[0] = m[ 0] * translation.x + m[ 4] * translation.y + m[ 8] * translation.z + m[12];
		tmp[1] = m[ 1] * translation.x + m[ 5] * translation.y + m[ 9] * translation.z + m[13];
		tmp[2] = m[ 2] * translation.x + m[ 6] * translation.y + m[10] * translation.z + m[14];
		tmp[3] = m[ 3] * translation.x + m[ 7] * translation.y + m[11] * translation.z + m[15];
		
		m[12] = tmp[0];
		m[13] = tmp[1];
		m[14] = tmp[2];
		m[15] = tmp[3];
	}
	
	RN_INLINE void Matrix::Translate(const Vector4 &translation)
	{
		float tmp[4];
		
		tmp[0] = m[ 0] * translation.x + m[ 4] * translation.y + m[ 8] * translation.z + m[12] * translation.w;
		tmp[1] = m[ 1] * translation.x + m[ 5] * translation.y + m[ 9] * translation.z + m[13] * translation.w;
		tmp[2] = m[ 2] * translation.x + m[ 6] * translation.y + m[10] * translation.z + m[14] * translation.w;
		tmp[3] = m[ 3] * translation.x + m[ 7] * translation.y + m[11] * translation.z + m[15] * translation.w;
		
		m[12] = tmp[0];
		m[13] = tmp[1];
		m[14] = tmp[2];
		m[15] = tmp[3];
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
