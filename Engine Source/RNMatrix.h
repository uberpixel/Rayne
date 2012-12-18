//
//  RNMatrix.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MATRIX_H__
#define __RAYNE_MATRIX_H__

#include "RNBase.h"
#include "RNVector.h"

namespace RN
{
	class Matrix
	{
	public:
		Matrix();
		Matrix(const Matrix& other);

		Matrix& Matrix::operator*= (const Matrix& other);
		Matrix Matrix::operator* (const Matrix& other) const;
		Vector3 Matrix::operator* (const Vector3& other) const;
		Vector4 Matrix::operator* (const Vector4& other) const;

		void SetIdentity();
		float Determinant() const;
		float DeterminantSubmatrix(const int k) const;

		Matrix Matrix::Inverse() const;

		struct
		{
			float m[16];
		};
	};

	RN_INLINE Matrix::Matrix()
	{
		int i;
		for(i = 0; i < 16; i++) m[i] = 0.0f;
	}

	RN_INLINE Matrix::Matrix(const Matrix& other)
	{
		int i;
		for(i = 0; i < 16; i++) m[i] = other.m[i];
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

	RN_INLINE void Matrix::SetIdentity()
	{
		m[0] = 1.0f;
		m[1] = 0.0f;
		m[2] = 0.0f;
		m[3] = 0.0f;
		m[4] = 0.0f;
		m[5] = 1.0f;
		m[6] = 0.0f;
		m[7] = 0.0f;
		m[8] = 0.0f;
		m[9] = 0.0f;
		m[10] = 1.0f;
		m[11] = 0.0f;
		m[12] = 0.0f;
		m[13] = 0.0f;
		m[14] = 0.0f;
		m[15] = 1.0f;
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

}

#endif /* __RAYNE_MATRIX_H__ */
