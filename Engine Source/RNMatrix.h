//
//  RNMatrix.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MATRIX_H__
#define __RAYNE_MATRIX_H__

#include "RNBase.h"
#include "RNVector.h"
#include "RNMatrixQuaternion.h"

namespace RN
{
	RN_INLINE Matrix::Matrix()
	{
		MakeIdentity();
	}

	RN_INLINE Matrix& Matrix::operator*= (const Matrix& other)
	{
#if RN_SIMD
		alignas(16) float tmp[16];
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

	RN_INLINE Matrix Matrix::operator* (const Matrix& other) const
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


	RN_INLINE Vector3 Matrix::operator* (const Vector3& other) const
	{
		Vector3 result;

		result.x = m[0] * other.x + m[4] * other.y + m[ 8] * other.z + m[12];
		result.y = m[1] * other.x + m[5] * other.y + m[ 9] * other.z + m[13];
		result.z = m[2] * other.x + m[6] * other.y + m[10] * other.z + m[14];

		return result;
	}

	RN_INLINE Vector4 Matrix::operator* (const Vector4& other) const
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
	
	RN_INLINE Matrix Matrix::GetInverse() const
	{
		Matrix result;
		
		float det = GetDeterminant();
		
		for(int i=0; i<16; i++)
		{
			result.m[i] = GetDeterminantSubmatrix(i) / det;
		}
		
		return result;
	}

	RN_INLINE void Matrix::MakeIdentity()
	{
		std::fill(m, m + 16, 0);
		m[0] = m[5] = m[10] = m[15] = 1.0f;
	}
	
	RN_INLINE void Matrix::MakeTranslate(const Vector3& trans)
	{
		MakeIdentity();
		
		m[12] = trans.x;
		m[13] = trans.y;
		m[14] = trans.z;
	}
	
	RN_INLINE void Matrix::MakeTranslate(const Vector4& trans)
	{
		MakeIdentity();
		
		m[12] = trans.x;
		m[13] = trans.y;
		m[14] = trans.z;
		m[15] = trans.w;
	}
	
	RN_INLINE void Matrix::MakeScale(const Vector3& scal)
	{
		MakeIdentity();
		
		m[0] = scal.x;
		m[5] = scal.y;
		m[10] = scal.z;
	}
	
	RN_INLINE void Matrix::MakeScale(const Vector4& scal)
	{
		MakeIdentity();
		
		m[0] = scal.x;
		m[5] = scal.y;
		m[10] = scal.z;
		m[15] = scal.w;
	}
	
	RN_INLINE void Matrix::MakeRotate(const Vector3& rot)
	{
		Quaternion quat(rot);
		*this = quat.GetRotationMatrix();
	}
	
	RN_INLINE void Matrix::MakeRotate(const Vector4& rot)
	{
		Quaternion quat(rot);
		*this = quat.GetRotationMatrix();
	}
	
	RN_INLINE void Matrix::MakeRotate(const Quaternion& rot)
	{
		Quaternion quat(rot);
		*this = quat.GetRotationMatrix();
	}

	
	RN_INLINE void Matrix::MakeProjectionOrthogonal(float left, float right, float bottom, float top, float clipnear, float clipfar)
	{
		MakeIdentity();
		
		float rl = right - left;
		float tb = top - bottom;
		float fn = clipfar - clipnear;
		float tx = - (right + left) / (right - left);
		float ty = - (top + bottom) / (top - bottom);
		float tz = - (clipfar + clipnear) / (clipfar - clipnear);
		
		m[0] = 2.0f / rl;
		m[5] = 2.0f / tb;
		m[10] = -2.0f / fn;
		
		m[12] = tx;
		m[13] = ty;
		m[14] = tz;
		m[15] = 1.0f;
	}
	
	RN_INLINE void Matrix::MakeProjectionPerspective(float arc, float aspect, float clipnear, float clipfar)
	{
		MakeIdentity();
		
		float xFac, yFac;
		yFac = tanf(arc * k::Pi / 360.0f);
		xFac = yFac * aspect;
		
		m[0] = 1.0f / xFac;
		m[5] = 1.0f / yFac;
		m[10] = -(clipfar + clipnear) / (clipfar - clipnear);
		m[11] = -1.0f;
		m[14] = -(2.0f * clipfar * clipnear) / (clipfar - clipnear);
		m[15] = 0.0f;
	}
	
	RN_INLINE void Matrix::MakeInverseProjectionPerspective(float arc, float aspect, float clipnear, float clipfar)
	{
		MakeIdentity();
		
		float xFac, yFac;
		yFac = tanf(arc * k::Pi / 360.0f);
		xFac = yFac * aspect;
		
		m[0] = xFac;
		m[5] = yFac;
		m[10] = 0.0f;
		m[11] = -(clipfar - clipnear) / (2.0f * clipfar * clipnear);
		m[14] = -1.0f;
		m[15] = (clipfar + clipnear) / (2.0f * clipfar * clipnear);
	}
	
	RN_INLINE float Matrix::GetDeterminant() const
	{
		float det = m[0] * (m[5]*m[10]*m[15] + m[6]*m[11]*m[13] + m[7]*m[9]*m[14] - m[7]*m[10]*m[13] - m[6]*m[9]*m[15] - m[5]*m[11]*m[14]);
		det -= m[1] * (m[4]*m[10]*m[15] + m[6]*m[11]*m[12] + m[7]*m[8]*m[14] - m[7]*m[10]*m[12] - m[6]*m[8]*m[15] - m[4]*m[11]*m[14]);
		det += m[2] * (m[4]*m[9]*m[15] + m[5]*m[11]*m[12] + m[7]*m[8]*m[13] - m[7]*m[9]*m[12] - m[5]*m[8]*m[15] - m[4]*m[11]*m[13]);
		det -= m[3] * (m[4]*m[9]*m[14] + m[5]*m[10]*m[12] + m[6]*m[8]*m[13] - m[6]*m[9]*m[12] - m[5]*m[8]*m[14] - m[4]*m[10]*m[13]);
		return det;
	}

	RN_INLINE float Matrix::GetDeterminantSubmatrix(const int k) const
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
		
		result.y = asin(fmax(fmin(-m[9], 1.0), -1.0));
		double cy = cos(result.y);
		if(Math::FastAbs(cy) > k::EpsilonFloat)
		{
			result.x = atan2(m[8]/cy, m[10]/cy);
			result.z = atan2(m[1]/cy, m[5]/cy);
		}
		else
		{
			result.z = 0.0f;
			if(result.y > 0.0f)
			{
				result.x = atan2(m[4], m[0]);
			}
			else
			{
				result.x = atan2(-m[4], -m[0]);
			}
		}
		
		result *= 180.0f / k::Pi;
		return result;
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
	
	RN_INLINE void Matrix::SetTranslationComponents(const Vector3& trans)
	{
		m[12] = trans.x;
		m[13] = trans.y;
		m[14] = trans.z;
	}
	
	RN_INLINE void Matrix::SetTranslationComponents(const Vector4& trans)
	{
		m[12] = trans.x;
		m[13] = trans.y;
		m[14] = trans.z;
		m[15] = trans.w;
	}
	
	RN_INLINE void Matrix::SetScaleComponents(const Vector3& scal)
	{
		m[0] = scal.x;
		m[5] = scal.y;
		m[10] = scal.z;
	}
	
	RN_INLINE void Matrix::SetScaleComponents(const Vector4& scal)
	{
		m[0] = scal.x;
		m[5] = scal.y;
		m[10] = scal.z;
		m[15] = scal.w;
	}
	
	RN_INLINE void Matrix::Translate(const Vector3& trans)
	{
#if RN_SIMD
		SIMD::VecFloat result = SIMD::Mul(vec[0], SIMD::Set(trans.x));
		result = SIMD::Add(result, SIMD::Mul(vec[1], SIMD::Set(trans.y)));
		result = SIMD::Add(result, SIMD::Mul(vec[2], SIMD::Set(trans.z)));
		
		vec[3] = SIMD::Add(result, vec[3]);
#else
		float tmp[4];
		
		tmp[0] = m[ 0] * trans.x + m[ 4] * trans.y + m[ 8] * trans.z + m[12];
		tmp[1] = m[ 1] * trans.x + m[ 5] * trans.y + m[ 9] * trans.z + m[13];
		tmp[2] = m[ 2] * trans.x + m[ 6] * trans.y + m[10] * trans.z + m[14];
		tmp[3] = m[ 3] * trans.x + m[ 7] * trans.y + m[11] * trans.z + m[15];
		
		m[12] = tmp[0];
		m[13] = tmp[1];
		m[14] = tmp[2];
		m[15] = tmp[3];
#endif
	}
	
	RN_INLINE void Matrix::Translate(const Vector4& trans)
	{
#if RN_SIMD
		SIMD::VecFloat result = SIMD::Mul(vec[0], SIMD::Set(trans.x));
		result = SIMD::Add(result, SIMD::Mul(vec[1], SIMD::Set(trans.y)));
		result = SIMD::Add(result, SIMD::Mul(vec[2], SIMD::Set(trans.z)));
		
		vec[3] = SIMD::Add(result, SIMD::Mul(vec[3], SIMD::Set(trans.w)));
#else
		float tmp[4];
		
		tmp[0] = m[ 0] * trans.x + m[ 4] * trans.y + m[ 8] * trans.z + m[12] * trans.w;
		tmp[1] = m[ 1] * trans.x + m[ 5] * trans.y + m[ 9] * trans.z + m[13] * trans.w;
		tmp[2] = m[ 2] * trans.x + m[ 6] * trans.y + m[10] * trans.z + m[14] * trans.w;
		tmp[3] = m[ 3] * trans.x + m[ 7] * trans.y + m[11] * trans.z + m[15] * trans.w;
		
		m[12] = tmp[0];
		m[13] = tmp[1];
		m[14] = tmp[2];
		m[15] = tmp[3];
#endif
	}
	
	RN_INLINE void Matrix::Scale(const Vector3& scal)
	{
		Matrix temp;
		temp.MakeScale(scal);
		
		*this *= temp;
	}
	
	RN_INLINE void Matrix::Scale(const Vector4& scal)
	{
		Matrix temp;
		temp.MakeScale(scal);
		
		*this *= temp;
	}
	
	RN_INLINE void Matrix::Rotate(const Vector3& rot)
	{
		Matrix temp;
		temp.MakeRotate(rot);
		
		*this *= temp;
	}
	
	RN_INLINE void Matrix::Rotate(const Vector4& rot)
	{
		Matrix temp;
		temp.MakeRotate(rot);
		
		*this *= temp;
	}
	
	RN_INLINE void Matrix::Rotate(const Quaternion& rot)
	{
		Matrix temp;
		temp.MakeRotate(rot);
		
		*this *= temp;
	}
	
	
	RN_INLINE Vector3 Matrix::GetTransformedVector(const Vector3& other) const
	{
		Vector3 result;
		result = (*this) * other;
		return result;
	}
	
	RN_INLINE Vector4 Matrix::GetTransformedVector(const Vector4& other) const
	{
		Vector4 result;
		result = (*this) * other;
		return result;
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
}

#endif /* __RAYNE_MATRIX_H__ */
