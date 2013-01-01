//
//  RNMatrix.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
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
		float tmp[4];
		for(int i = 0; i < 4; i++)
		{
			int k = i*4;
			tmp[0] = m[k];
			tmp[1] = m[k+1];
			tmp[2] = m[k+2];
			tmp[3] = m[k+3];
			m[k] = tmp[0]*other.m[0] + tmp[1]*other.m[4] + tmp[2]*other.m[8] + tmp[3]*other.m[12];
			m[k+1] = tmp[0]*other.m[1] + tmp[1]*other.m[5] + tmp[2]*other.m[9] + tmp[3]*other.m[13];
			m[k+2] = tmp[0]*other.m[2] + tmp[1]*other.m[6] + tmp[2]*other.m[10] + tmp[3]*other.m[14];
			m[k+3] = tmp[0]*other.m[3] + tmp[1]*other.m[7] + tmp[2]*other.m[11] + tmp[3]*other.m[15];
		}

		return *this;
	}

	RN_INLINE Matrix Matrix::operator* (const Matrix& other) const
	{
		Matrix matrix;
		for(int i = 0; i < 4; i++)
		{
			int k = i*4;
			matrix.m[k] = m[0]*other.m[0] + m[1]*other.m[4] + m[2]*other.m[8] + m[3]*other.m[12];
			matrix.m[k+1] = m[0]*other.m[1] + m[1]*other.m[5] + m[2]*other.m[9] + m[3]*other.m[13];
			matrix.m[k+2] = m[0]*other.m[2] + m[1]*other.m[6] + m[2]*other.m[10] + m[3]*other.m[14];
			matrix.m[k+3] = m[0]*other.m[3] + m[1]*other.m[7] + m[2]*other.m[11] + m[3]*other.m[15];
		}

		return matrix;
	}


	RN_INLINE Vector3 Matrix::operator* (const Vector3& vec) const
	{
		Vector3 result;

		result.x = m[0]*vec.x + m[1]*vec.y + m[2]*vec.z;
		result.y = m[4]*vec.x + m[5]*vec.y + m[6]*vec.z;
		result.z = m[8]*vec.x + m[9]*vec.y + m[10]*vec.z;

		return result;
	}

	RN_INLINE Vector4 Matrix::operator* (const Vector4& vec) const
	{
		Vector4 result;

		result.x = m[0]*vec.x + m[1]*vec.y + m[2]*vec.z + m[3]*vec.w;
		result.y = m[4]*vec.x + m[5]*vec.y + m[6]*vec.z + m[7]*vec.w;
		result.z = m[8]*vec.x + m[9]*vec.y + m[10]*vec.z + m[11]*vec.w;
		result.w = m[12]*vec.x + m[13]*vec.y + m[14]*vec.z + m[15]*vec.w;

		return result;
	}

	RN_INLINE Matrix Matrix::Inverse() const
	{
		Matrix matrix;
		float det = Determinant();
		// if(fabs(det) < kRNEpsilonFloat) error;

		for(int i = 0; i < 16; i++)
		{
			matrix.m[i] = DeterminantSubmatrix(i)/det;
		}

		return matrix;
	}

	RN_INLINE void Matrix::MakeIdentity()
	{
		memset(m, 0, 16 * sizeof(float));
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
		*this = quat.RotationMatrix();
	}
	
	RN_INLINE void Matrix::MakeRotate(const Vector4& rot)
	{
		Quaternion quat(rot);
		*this = quat.RotationMatrix();
	}
	
	RN_INLINE void Matrix::MakeRotate(const Quaternion& rot)
	{
		Quaternion quat(rot);
		*this = quat.RotationMatrix();
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
		m[5] = 2.0 / tb;
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
		yFac = tanf(arc * M_PI / 360.0f);
		xFac = yFac * aspect;
		
		m[0] = 1.0f / xFac;
		m[5] = 1.0f / yFac;
		m[10] = -(clipfar + clipnear) / (clipfar - clipnear);
		m[11] = -1.0f;
		m[14] = -(2.0f * clipfar * clipnear) / (clipfar - clipnear);
	}
	
	RN_INLINE void Matrix::MakeInverveProjectionPerspective(float arc, float aspect, float clipnear, float clipfar)
	{
		MakeIdentity();
		
		float xFac, yFac;
		yFac = tanf(arc * M_PI / 360.0f);
		xFac = yFac * aspect;
		
		m[0] = xFac;
		m[5] = yFac;
		m[11] = -(clipfar - clipnear) / (2.0f * clipfar * clipnear);
		m[14] = -1.0f;
		m[15] = (clipfar + clipnear) / (2.0f * clipfar * clipnear);
	}
	
	
	RN_INLINE float Matrix::Determinant() const
	{
		float det = m[0] * (m[5]*m[10]*m[15] + m[6]*m[11]*m[13] + m[7]*m[9]*m[14] - m[7]*m[10]*m[13] - m[6]*m[9]*m[15] - m[5]*m[11]*m[14]);
		det -= m[1] * (m[4]*m[10]*m[15] + m[6]*m[11]*m[12] + m[7]*m[8]*m[14] - m[7]*m[10]*m[12] - m[6]*m[8]*m[15] - m[4]*m[11]*m[14]);
		det += m[2] * (m[4]*m[9]*m[15] + m[5]*m[11]*m[12] + m[7]*m[8]*m[13] - m[7]*m[9]*m[12] - m[5]*m[8]*m[15] - m[4]*m[11]*m[13]);
		det -= m[3] * (m[4]*m[9]*m[14] + m[5]*m[10]*m[12] + m[6]*m[8]*m[13] - m[6]*m[9]*m[12] - m[5]*m[8]*m[14] - m[4]*m[10]*m[13]);
		return det;
	}

	RN_INLINE float Matrix::DeterminantSubmatrix(const int k) const
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
	
	
	RN_INLINE void Matrix::Translate(const Vector3& trans)
	{
		Matrix temp;
		temp.MakeTranslate(trans);
		
		*this *= temp;
	}
	
	RN_INLINE void Matrix::Translate(const Vector4& trans)
	{
		Matrix temp;
		temp.MakeTranslate(trans);
		
		*this *= temp;
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
	
	
	RN_INLINE Vector3 Matrix::Transform(const Vector3& other) const
	{
		Vector3 result;
		result = (*this) * other;
		return result;
	}
	
	RN_INLINE Vector4 Matrix::Transform(const Vector4& other) const
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
		
		memcpy(m, temp, 16 * sizeof(float));
	}
}

#endif /* __RAYNE_MATRIX_H__ */
