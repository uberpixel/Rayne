//
//  RNColor.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_Color_H__
#define __RAYNE_Color_H__

namespace RN
{
	class Color
	{
	public:
		RNAPI Color(float n=1.0f);
		RNAPI Color(float r, float g, float b, float a=1.0f);
		RNAPI Color(int r, int g, int b, int a=255);

		RNAPI bool operator== (const Color& other);
		RNAPI bool operator!= (const Color& other);

		RNAPI Color operator- () const;

		RNAPI Color& operator+= (const Color& other);
		RNAPI Color& operator-= (const Color& other);
		RNAPI Color& operator*= (const Color& other);
		RNAPI Color& operator/= (const Color& other);

		RNAPI Color operator+ (const Color& other) const;
		RNAPI Color operator- (const Color& other) const;
		RNAPI Color operator* (const Color& other) const;
		RNAPI Color operator/ (const Color& other) const;

		RNAPI Color& operator+= (float other);
		RNAPI Color& operator-= (float other);
		RNAPI Color& operator*= (float other);
		RNAPI Color& operator/= (float other);

		RNAPI Color operator+ (float other) const;
		RNAPI Color operator- (float other) const;
		RNAPI Color operator* (float other) const;
		RNAPI Color operator/ (float other) const;

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
	
	RN_INLINE Color::Color(int _r, int _g, int _b, int _a)
	{
		r = _r / 255.0f;
		g = _g / 255.0f;
		b = _b / 255.0f;
		a = _a / 255.0f;
	}


	RN_INLINE bool Color::operator== (const Color& other)
	{
		float absR = Math::FastAbs(r - other.r);
		float absG = Math::FastAbs(g - other.g);
		float absB = Math::FastAbs(b - other.b);
		float absA = Math::FastAbs(a - other.a);

		return (absR <= kRNEpsilonFloat && absG <= kRNEpsilonFloat && absB <= kRNEpsilonFloat && absA <= kRNEpsilonFloat);
	}

	RN_INLINE bool Color::operator!= (const Color& other)
	{
		float absR = Math::FastAbs(r - other.r);
		float absG = Math::FastAbs(g - other.g);
		float absB = Math::FastAbs(b - other.b);
		float absA = Math::FastAbs(a - other.a);

		return (absR > kRNEpsilonFloat || absG > kRNEpsilonFloat || absB > kRNEpsilonFloat || absA > kRNEpsilonFloat);
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

	#ifndef __GNUG__
	static_assert(std::is_trivially_copyable<Color>::value, "Color must be trivially copyable");
	#endif
}

#endif /* __RAYNE_Color_H__ */
