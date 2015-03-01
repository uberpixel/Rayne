//
//  RNColor.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_COLOR_H__
#define __RAYNE_COLOR_H__

#include "RNVector.h"

namespace RN
{
	class Color
	{
	public:
		Color(float n=1.0f);
		Color(float r, float g, float b, float a=1.0f);
		Color(int r, int g, int b, int a=255);

		bool operator== (const Color &other) const;
		bool operator!= (const Color &other) const;

		Color operator- () const;

		Color &operator+= (const Color &other);
		Color &operator-= (const Color &other);
		Color &operator*= (const Color &other);
		Color &operator/= (const Color &other);

		Color operator+ (const Color &other) const;
		Color operator- (const Color &other) const;
		Color operator* (const Color &other) const;
		Color operator/ (const Color &other) const;

		Color &operator+= (float other);
		Color &operator-= (float other);
		Color &operator*= (float other);
		Color &operator/= (float other);

		Color operator+ (float other) const;
		Color operator- (float other) const;
		Color operator* (float other) const;
		Color operator/ (float other) const;
		
		Vector4 GetHSV() const;
		
		static Color Red() { return Color(1.0f, 0.0f, 0.0f); }
		static Color Green() { return Color(0.0f, 1.0f, 0.0f); }
		static Color Blue() { return Color(0.0f, 0.0f, 1.0f); }
		static Color Yellow() { return Color(1.0f, 1.0f, 0.0f); }
		static Color Black() { return Color(0.0f, 0.0f, 0.0f); }
		static Color White() { return Color(1.0f, 1.0f, 1.0f); }
		static Color Gray() { return Color(0.5f, 0.5f, 0.5f); }
		static Color ClearColor() { return Color(0.0f, 0.0f, 0.0f, 0.0f); }
		static Color WithRGBA(float r, float g, float b, float alpha=1.0f) { return Color(r, g, b, alpha); }
		static Color WithHSV(float h, float s, float v, float alpha=1.0f);
		static Color WithHSV(const Vector4 &hsva);
		
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


	RN_INLINE bool Color::operator== (const Color &other) const
	{
		float absR = Math::FastAbs(r - other.r);
		float absG = Math::FastAbs(g - other.g);
		float absB = Math::FastAbs(b - other.b);
		float absA = Math::FastAbs(a - other.a);

		return (absR <= k::EpsilonFloat && absG <= k::EpsilonFloat && absB <= k::EpsilonFloat && absA <= k::EpsilonFloat);
	}

	RN_INLINE bool Color::operator!= (const Color &other) const
	{
		float absR = Math::FastAbs(r - other.r);
		float absG = Math::FastAbs(g - other.g);
		float absB = Math::FastAbs(b - other.b);
		float absA = Math::FastAbs(a - other.a);

		return (absR > k::EpsilonFloat || absG > k::EpsilonFloat || absB > k::EpsilonFloat || absA > k::EpsilonFloat);
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

	RN_INLINE Color &Color::operator+= (const Color &other)
	{
		r += other.r;
		g += other.g;
		b += other.b;
		a += other.a;

		return *this;
	}

	RN_INLINE Color &Color::operator-= (const Color &other)
	{
		r -= other.r;
		g -= other.g;
		b -= other.b;
		a -= other.a;

		return *this;
	}

	RN_INLINE Color &Color::operator*= (const Color &other)
	{
		r *= other.r;
		g *= other.g;
		b *= other.b;
		a *= other.a;

		return *this;
	}

	RN_INLINE Color &Color::operator/= (const Color &other)
	{
		r /= other.r;
		g /= other.g;
		b /= other.b;
		a /= other.a;

		return *this;
	}



	RN_INLINE Color Color::operator+ (const Color &other) const
	{
		Color result(*this);

		result.r += other.r;
		result.g += other.g;
		result.b += other.b;
		result.a += other.a;

		return result;
	}

	RN_INLINE  Color Color::operator- (const Color &other) const
	{
		Color result(*this);

		result.r -= other.r;
		result.g -= other.g;
		result.b -= other.b;
		result.a -= other.a;

		return result;
	}

	RN_INLINE Color Color::operator* (const Color &other) const
	{
		Color result(*this);

		result.r *= other.r;
		result.g *= other.g;
		result.b *= other.b;
		result.a *= other.a;

		return result;
	}

	RN_INLINE Color Color::operator/ (const Color &other) const
	{
		Color result(*this);

		result.r /= other.r;
		result.g /= other.g;
		result.b /= other.b;
		result.a /= other.a;

		return result;
	}



	RN_INLINE Color &Color::operator+= (float other)
	{
		r += other;
		g += other;
		b += other;
		a += other;

		return *this;
	}

	RN_INLINE Color &Color::operator-= (float other)
	{
		r -= other;
		g -= other;
		b -= other;
		a -= other;

		return *this;
	}

	RN_INLINE Color &Color::operator*= (float other)
	{
		r *= other;
		g *= other;
		b *= other;
		a *= other;

		return *this;
	}

	RN_INLINE Color &Color::operator/= (float other)
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
	
	RN_INLINE Color Color::WithHSV(float h, float s, float v, float alpha)
	{
		float hi = h * 3.0 / k::Pi;
		float f  = hi - floorf(hi);
		
		if(hi >= 3.0f)
			hi -= 6.0f;
		if(hi < -3.0f)
			hi += 6.0f;
		
		Color components(0.0, s, s * f, s * (1.0 - f));
		components = Color::White() - components;
		components *= v;
		components.r = fmaxf(components.r, 0.0f);
		components.g = fmaxf(components.g, 0.0f);
		components.b = fmaxf(components.b, 0.0f);
		components.a = fmaxf(components.a, 0.0f);
		
		if(hi < -2.0f)
		{
			return Color(components.r, components.a, components.g, alpha);
		}
		else if(hi < -1.0f)
		{
			return Color(components.b, components.r, components.g, alpha);
		}
		else if(hi < 0.0f)
		{
			return Color(components.g, components.r, components.a, alpha);
		}
		else if(hi < 1.0f)
		{
			return Color(components.g, components.b, components.r, alpha);
		}
		else if(hi < 2.0f)
		{
			return Color(components.a, components.g, components.r, alpha);
		}
		else
		{
			return Color(components.r, components.g, components.b, alpha);
		}
	}
	
	RN_INLINE Color Color::WithHSV(const Vector4 &hsva)
	{
		return WithHSV(hsva.x, hsva.y, hsva.z, hsva.w);
	}
	
	RN_INLINE Vector4 Color::GetHSV() const
	{
		float max = std::max(r, std::max(g, b));
		float min = std::min(r, std::min(g, b));
		float diff = max - min;
		
		float h = 0.0f;
		float s = 0.0f;
		float v = max;
		
		if(!Math::Compare(max, min))
		{
			if(Math::Compare(max, r))
			{
				h = k::Pi/3.0f * (g - b) / diff;
			}
			else if(Math::Compare(max, g))
			{
				h = k::Pi/3.0f * (2.0f + (b - r) / diff);
			}
			else if(Math::Compare(max, b))
			{
				h = k::Pi/3.0f * (4.0f + (r - g) / diff);
			}
			
			if(h < 0.0f)
			{
				h += 2.0f * k::Pi;
			}
		}
		
		if(!Math::Compare(max, 0.0f))
		{
			s = diff/max;
		}
		
		return Vector4(h - k::Pi, s, v, a);
	}

#ifndef __GNUG__
	static_assert(std::is_trivially_copyable<Color>::value, "Color must be trivially copyable");
#endif
}

#endif /* __RAYNE_COLOR_H__ */
