//
//  RNRect.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_RECT_H_
#define __RAYNE_RECT_H_

#include "../Base/RNBase.h"
#include "RNMath.h"
#include "RNVector.h"

namespace RN
{
	class Rect
	{
	public:
		Rect();
		Rect(float x, float y, float width, float height);
		Rect(const Vector2 &origin, float width, float height);
		Rect(const Vector2 &origin, const Vector2 &size);
		Rect(const Rect &other);

		bool operator== (const Rect &other) const;
		bool operator!= (const Rect &other) const;

		bool ContainsPoint(const Vector2 &point) const;
		bool IntersectsRect(const Rect &other) const;
		bool ContainsRect(const Rect &other) const;

		Rect &Inset(float dx, float dy);
		Rect &Integral();

		Rect GetIntegral() const;

		float GetTop() const;
		float GetBottom() const;
		float GetLeft() const;
		float GetRight() const;

		Vector2 GetOrigin() const
		{
			return Vector2(x, y);
		}

		Vector2 GetSize() const
		{
			return Vector2(width, height);
		}

		struct
		{
			float x;
			float y;

			float width;
			float height;
		};
	};

	RN_INLINE Rect::Rect()
	{
		x = y = width = height = 0.0f;
	}

	RN_INLINE Rect::Rect(float tx, float ty, float twidth, float theight)
	{
		x = tx;
		y = ty;

		width  = twidth;
		height = theight;
	}

	RN_INLINE Rect::Rect(const Vector2 &origin, float twidth, float theight)
	{
		x = origin.x;
		y = origin.y;

		width  = twidth;
		height = theight;
	}

	RN_INLINE Rect::Rect(const Vector2 &origin, const Vector2 &size)
	{
		x = origin.x;
		y = origin.y;

		width  = size.x;
		height = size.y;
	}

	RN_INLINE Rect::Rect(const Rect &other)
	{
		x = other.x;
		y = other.y;

		width  = other.width;
		height = other.height;
	}

	RN_INLINE bool Rect::operator== (const Rect &other) const
	{
		if(Math::FastAbs(x - other.x) > k::EpsilonFloat || Math::FastAbs(y - other.y) > k::EpsilonFloat ||
			Math::FastAbs(width - other.width) > k::EpsilonFloat || Math::FastAbs(height - other.height) > k::EpsilonFloat)
			return false;

		return true;
	}

	RN_INLINE bool Rect::operator!= (const Rect &other) const
	{
		if(Math::FastAbs(x - other.x) > k::EpsilonFloat || Math::FastAbs(y - other.y) > k::EpsilonFloat ||
			Math::FastAbs(width - other.width) > k::EpsilonFloat || Math::FastAbs(height - other.height) > k::EpsilonFloat)
			return true;

		return false;
	}


	RN_INLINE bool Rect::ContainsPoint(const Vector2 &point) const
	{
		return ((point.x >= x && point.x <= x + width) && (point.y >= y && point.y <= y + height));
	}

	RN_INLINE bool Rect::IntersectsRect(const Rect &other) const
	{
		return ((x < other.x + other.width && x + width > other.x) &&
				(y < other.y + other.height && y + height > other.y));
	}

	RN_INLINE bool Rect::ContainsRect(const Rect &other) const
	{
		return ((x <= other.x && x + width >= other.x + other.width) && (y <= other.y && y + height >= other.y + other.height));
	}


	RN_INLINE Rect &Rect::Inset(float dx, float dy)
	{
		x += dx;
		y += dy;

		width  -= dx * 2;
		height -= dy * 2;

		return *this;
	}

	RN_INLINE Rect &Rect::Integral()
	{
		x = floorf(x);
		y = floorf(y);

		width  = floorf(width);
		height = floorf(height);

		return *this;
	}

	RN_INLINE Rect Rect::GetIntegral() const
	{
		return Rect(*this).Integral();
	}



	RN_INLINE float Rect::GetTop() const
	{
		return y;
	}

	RN_INLINE float Rect::GetBottom() const
	{
		return y + height;
	}

	RN_INLINE float Rect::GetLeft() const
	{
		return x;
	}

	RN_INLINE float Rect::GetRight() const
	{
		return x + width;
	}
}


#endif /* __RAYNE_RECT_H_ */
