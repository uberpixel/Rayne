//
//  RNColor.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_Color_H__
#define __RAYNE_Color_H__

namespace RN
{
	class Color
	{
	public:
		Color(float n=1.0f);
		Color(float r, float g, float b, float a=1.0f);
		Color(const Color& other);
		
		bool operator== (const Color& other);
		bool operator!= (const Color& other);
		
		Color& operator= (const Color& other);
		Color operator- () const;
		
		Color& operator+= (const Color& other);
		Color& operator-= (const Color& other);
		Color& operator*= (const Color& other);
		Color& operator/= (const Color& other);
		
		Color operator+ (const Color& other) const;
		Color operator- (const Color& other) const;
		Color operator* (const Color& other) const;
		Color operator/ (const Color& other) const;
		
		Color& operator+= (float other);
		Color& operator-= (float other);
		Color& operator*= (float other);
		Color& operator/= (float other);
		
		Color operator+ (float other) const;
		Color operator- (float other) const;
		Color operator* (float other) const;
		Color operator/ (float other) const;
		
		struct
		{
			float r;
			float g;
			float b;
			float a;
		};
	};
	
	RN_INLINE Color::Color(float n)
	{
		r = g = b = a = n;
	}
	
	RN_INLINE Color::Color(float _r, float _g, float _b, float _a)
	{
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}
	
	RN_INLINE Color::Color(const Color& other)
	{
		r = other.r;
		g = other.g;
		b = other.b;
		a = other.a;
	}
	
	
	RN_INLINE bool Color::operator== (const Color& other)
	{
		float absR = fabsf(r - other.r);
		float absG = fabsf(g - other.g);
		float absB = fabsf(b - other.b);
		float absA = fabsf(a - other.a);
		
		return (absR <= kRNEpsilonFloat && absG <= kRNEpsilonFloat && absB <= kRNEpsilonFloat && absA <= kRNEpsilonFloat);
	}
	
	RN_INLINE bool Color::operator!= (const Color& other)
	{
		float absR = fabsf(r - other.r);
		float absG = fabsf(g - other.g);
		float absB = fabsf(b - other.b);
		float absA = fabsf(a - other.a);
		
		return (absR > kRNEpsilonFloat || absG > kRNEpsilonFloat || absB > kRNEpsilonFloat || absA > kRNEpsilonFloat);
	}
	
	RN_INLINE Color& Color::operator= (const Color& other)
	{
		r = other.r;
		g = other.g;
		b = other.b;
		a = other.a;
		
		return *this;
	}
	
	RN_INLINE Color Color::operator- () const
	{
		Color result(*this);
		
		result.r = -result.r;
		result.g = -result.g;
		result.b = -result.b;
		result.a = -result.a;
		
		return result;
	}
	
	RN_INLINE Color& Color::operator+= (const Color& other)
	{
		r += other.r;
		g += other.g;
		b += other.b;
		a += other.a;
		
		return *this;
	}
	
	RN_INLINE Color& Color::operator-= (const Color& other)
	{
		r -= other.r;
		g -= other.g;
		b -= other.b;
		a -= other.a;
		
		return *this;
	}
	
	RN_INLINE Color& Color::operator*= (const Color& other)
	{
		r *= other.r;
		g *= other.g;
		b *= other.b;
		a *= other.a;
		
		return *this;
	}
	
	RN_INLINE Color& Color::operator/= (const Color& other)
	{
		r /= other.r;
		g /= other.g;
		b /= other.b;
		a /= other.a;
		
		return *this;
	}
	
	
	
	RN_INLINE Color Color::operator+ (const Color& other) const
	{
		Color result(*this);
		
		result.r += other.r;
		result.g += other.g;
		result.b += other.b;
		result.a += other.a;
		
		return result;
	}
	
	RN_INLINE  Color Color::operator- (const Color& other) const
	{
		Color result(*this);
		
		result.r -= other.r;
		result.g -= other.g;
		result.b -= other.b;
		result.a -= other.a;
		
		return result;
	}
	
	RN_INLINE Color Color::operator* (const Color& other) const
	{
		Color result(*this);
		
		result.r *= other.r;
		result.g *= other.g;
		result.b *= other.b;
		result.a *= other.a;
		
		return result;
	}
	
	RN_INLINE Color Color::operator/ (const Color& other) const
	{
		Color result(*this);
		
		result.r /= other.r;
		result.g /= other.g;
		result.b /= other.b;
		result.a /= other.a;
		
		return result;
	}
	
	
	
	RN_INLINE Color& Color::operator+= (float other)
	{
		r += other;
		g += other;
		b += other;
		a += other;
		
		return *this;
	}
	
	RN_INLINE Color& Color::operator-= (float other)
	{
		r -= other;
		g -= other;
		b -= other;
		a -= other;
		
		return *this;
	}
	
	RN_INLINE Color& Color::operator*= (float other)
	{
		r *= other;
		g *= other;
		b *= other;
		a *= other;
		
		return *this;
	}
	
	RN_INLINE Color& Color::operator/= (float other)
	{
		r /= other;
		g /= other;
		b /= other;
		a /= other;
		
		return *this;
	}
	
	
	RN_INLINE Color Color::operator+ (float other) const
	{
		Color result(*this);
		
		result.r += other;
		result.g += other;
		result.b += other;
		result.a += other;
		
		return result;
	}
	
	RN_INLINE Color Color::operator- (float other) const
	{
		Color result(*this);
		
		result.r -= other;
		result.g -= other;
		result.b -= other;
		result.a -= other;
		
		return result;
	}
	
	RN_INLINE Color Color::operator* (float other) const
	{
		Color result(*this);
		
		result.r *= other;
		result.g *= other;
		result.b *= other;
		result.a *= other;
		
		return result;
	}
	
	RN_INLINE Color Color::operator/ (float other) const
	{
		Color result(*this);
		
		result.r /= other;
		result.g /= other;
		result.b /= other;
		result.a /= other;
		
		return result;
	}
}

#endif /* __RAYNE_Color_H__ */
