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

namespace rn
{
    class vector2
    {
    public:
        vector2();
        vector2(const vector2& other);
        vector2(const float n);
        vector2(const float x, const float y);
        
        bool operator== (const vector2 &other) const;
        bool operator!= (const vector2 &other) const;
		
		vector2 operator- () const;
		
		vector2 operator+ (const vector2& other) const;
		vector2 operator- (const vector2& other) const;
		vector2 operator* (const vector2& other) const;
		vector2 operator/ (const vector2& other) const;
		vector2 operator* (const float n) const;
		vector2 operator/ (const float n) const;
		
		vector2& operator+= (const vector2& other);
		vector2& operator-= (const vector2& other);
		vector2& operator*= (const vector2& other);
		vector2& operator/= (const vector2& other);
		
		float length() const;
        
		struct
		{
			float x;
			float y;
		};
    };
	
	RN_INLINE vector2::vector2()
	{
		x = y = 0.0f;
	}
	
	RN_INLINE vector2::vector2(const vector2& other)
	{
		x = other.x;
		y = other.y;
	}
	
	RN_INLINE vector2::vector2(const float n)
	{
		x = y = n;
	}
	
	RN_INLINE vector2::vector2(const float _x, const float _y)
	{
		x = _x;
		y = _y;
	}
    
    RN_INLINE bool vector2::operator== (const vector2 &other) const
    {
        if(fabs(x - other.x) > kRNEpsilonFloat)
            return false;
        
        if(fabs(y - other.y) > kRNEpsilonFloat)
            return false;
        
        return true;
    }
    
    RN_INLINE bool vector2::operator!= (const vector2 &other) const
    {
        if(fabs(x - other.x) <= kRNEpsilonFloat)
            return false;
        
        if(fabs(y - other.y) <= kRNEpsilonFloat)
            return false;
        
        return true;
    }
	
	RN_INLINE vector2 vector2::operator- () const
	{
		return vector2(-x, -y);
	}
	
	RN_INLINE vector2 vector2::operator+ (const vector2& other) const
	{
		return vector2(x + other.x, y + other.y);
	}
	RN_INLINE vector2 vector2::operator- (const vector2& other) const
	{
		return vector2(x - other.x, y - other.y);
	}
	RN_INLINE vector2 vector2::operator* (const vector2& other) const
	{
		return vector2(x * other.x, y * other.y);
	}
	RN_INLINE vector2 vector2::operator/ (const vector2& other) const
	{
		return vector2(x / other.x, y / other.y);
	}
	RN_INLINE vector2 vector2::operator* (const float n) const
	{
		return vector2(x * n, y * n);
	}
	RN_INLINE vector2 vector2::operator/ (const float n) const
	{
		return vector2(x / n, y / n);
	}
	
	RN_INLINE vector2& vector2::operator+= (const vector2& other)
	{
		x += other.x;
		y += other.y;
		
		return *this;
	}
	RN_INLINE vector2& vector2::operator-= (const vector2& other)
	{
		x -= other.x;
		y -= other.y;
		
		return *this;
	}
	RN_INLINE vector2& vector2::operator*= (const vector2& other)
	{
		x *= other.x;
		y *= other.y;
		
		return *this;
	}
	RN_INLINE vector2& vector2::operator/= (const vector2& other)
	{
		x /= other.x;
		y /= other.y;
		
		return *this;
	}
	
	RN_INLINE float vector2::length() const
	{
		return fabsf(x * x + y * y);
	}
}

#endif /* __RAYNE_VECTOR_H__ */
