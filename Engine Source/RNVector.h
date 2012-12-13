//
//  RNVector.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VECTOR_H__
#define __RAYNE_VECTOR_H__

#include "RNBase.h"

namespace RN
{
	class Vector2
	{
	public:
		Vector2();
		Vector2(const Vector2& other);
		Vector2(const float n);
		Vector2(const float x, const float y);
		
		bool operator== (const Vector2 &other) const;
		bool operator!= (const Vector2 &other) const;
		
		Vector2 operator- () const;
		
		Vector2 operator+ (const Vector2& other) const;
		Vector2 operator- (const Vector2& other) const;
		Vector2 operator* (const Vector2& other) const;
		Vector2 operator/ (const Vector2& other) const;
		Vector2 operator* (const float n) const;
		Vector2 operator/ (const float n) const;
		
		Vector2& operator+= (const Vector2& other);
		Vector2& operator-= (const Vector2& other);
		Vector2& operator*= (const Vector2& other);
		Vector2& operator/= (const Vector2& other);
		
		float Length() const;
		
		struct
		{
			float x;
			float y;
		};
	};
	
	RN_INLINE Vector2::Vector2()
	{
		x = y = 0.0f;
	}
	
	RN_INLINE Vector2::Vector2(const Vector2& other)
	{
		x = other.x;
		y = other.y;
	}
	
	RN_INLINE Vector2::Vector2(const float n)
	{
		x = y = n;
	}
	
	RN_INLINE Vector2::Vector2(const float _x, const float _y)
	{
		x = _x;
		y = _y;
	}
	
	RN_INLINE bool Vector2::operator== (const Vector2 &other) const
	{
		if(fabs(x - other.x) > kRNEpsilonFloat)
			return false;
		
		if(fabs(y - other.y) > kRNEpsilonFloat)
			return false;
		
		return true;
	}
	
	RN_INLINE bool Vector2::operator!= (const Vector2 &other) const
	{
		if(fabs(x - other.x) <= kRNEpsilonFloat)
			return false;
		
		if(fabs(y - other.y) <= kRNEpsilonFloat)
			return false;
		
		return true;
	}
	
	RN_INLINE Vector2 Vector2::operator- () const
	{
		return Vector2(-x, -y);
	}
	
	RN_INLINE Vector2 Vector2::operator+ (const Vector2& other) const
	{
		return Vector2(x + other.x, y + other.y);
	}
	RN_INLINE Vector2 Vector2::operator- (const Vector2& other) const
	{
		return Vector2(x - other.x, y - other.y);
	}
	RN_INLINE Vector2 Vector2::operator* (const Vector2& other) const
	{
		return Vector2(x * other.x, y * other.y);
	}
	RN_INLINE Vector2 Vector2::operator/ (const Vector2& other) const
	{
		return Vector2(x / other.x, y / other.y);
	}
	RN_INLINE Vector2 Vector2::operator* (const float n) const
	{
		return Vector2(x * n, y * n);
	}
	RN_INLINE Vector2 Vector2::operator/ (const float n) const
	{
		return Vector2(x / n, y / n);
	}
	
	RN_INLINE Vector2& Vector2::operator+= (const Vector2& other)
	{
		x += other.x;
		y += other.y;
		
		return *this;
	}
	RN_INLINE Vector2& Vector2::operator-= (const Vector2& other)
	{
		x -= other.x;
		y -= other.y;
		
		return *this;
	}
	RN_INLINE Vector2& Vector2::operator*= (const Vector2& other)
	{
		x *= other.x;
		y *= other.y;
		
		return *this;
	}
	RN_INLINE Vector2& Vector2::operator/= (const Vector2& other)
	{
		x /= other.x;
		y /= other.y;
		
		return *this;
	}
	
	RN_INLINE float Vector2::Length() const
	{
		return fabsf(x * x + y * y);
	}
}

#endif /* __RAYNE_VECTOR_H__ */
