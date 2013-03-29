//
//  RNRect.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RECT_H__
#define __RAYNE_RECT_H__

#include "RNBase.h"
#include "RNVector.h"

namespace RN
{
	class Rect
	{
	public:
		RNAPI Rect();
		RNAPI Rect(float x, float y, float width, float height);
		RNAPI Rect(const Vector2& origin, float width, float height);
		RNAPI Rect(const Vector2& origin, const Vector2& size);
		RNAPI Rect(const Rect& other);
		
		RNAPI bool operator== (const Rect& other);
		RNAPI bool operator!= (const Rect& other);
		
		RNAPI bool ContainsPoint(const Vector2& point) const;
		RNAPI bool IntersectsRect(const Rect& other) const;
		RNAPI bool ContainsRect(const Rect& other) const;
		
		RNAPI void Inset(float dx, float dy);
		
		RNAPI float Top() const;
		RNAPI float Bottom() const;
		RNAPI float Left() const;
		RNAPI float Right() const;
		
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
	
	RN_INLINE Rect::Rect(const Vector2& origin, float twidth, float theight)
	{
		x = origin.x;
		y = origin.y;
		
		width  = twidth;
		height = theight;
	}
	
	RN_INLINE Rect::Rect(const Vector2& origin, const Vector2& size)
	{
		x = origin.x;
		y = origin.y;
		
		width  = size.x;
		height = size.y;
	}
	
	RN_INLINE Rect::Rect(const Rect& other)
	{
		x = other.x;
		y = other.y;
		
		width  = other.width;
		height = other.height;
	}
	
	RN_INLINE bool Rect::operator== (const Rect& other)
	{
		if(abs(x - other.x) > kRNEpsilonFloat || abs(y - other.y) > kRNEpsilonFloat ||
		   abs(width - other.width) > kRNEpsilonFloat || abs(height - other.height) > kRNEpsilonFloat)
			return false;
		
		return true;
	}
	
	RN_INLINE bool Rect::operator!= (const Rect& other)
	{
		if(abs(x - other.x) > kRNEpsilonFloat || abs(y - other.y) > kRNEpsilonFloat ||
		   abs(width - other.width) > kRNEpsilonFloat || abs(height - other.height) > kRNEpsilonFloat)
			return true;
		
		return false;
	}
	
	
	RN_INLINE bool Rect::ContainsPoint(const Vector2& point) const
	{
		return ((x >= point.x && x <= point.x + width) && (y >= point.y && y <= point.y + height));
	}
	
	RN_INLINE bool Rect::IntersectsRect(const Rect& other) const
	{
		return ((x < other.x + other.width && x + width > other.x) &&
				(y < other.y + other.height && y + height > other.y));
	}
	
	RN_INLINE bool Rect::ContainsRect(const Rect& other) const
	{
		return ((x <= other.x && x + width >= other.x + other.width) && (y <= other.y && y + height >= other.x + other.height));
	}
	
	
	RN_INLINE void Rect::Inset(float dx, float dy)
	{
		float hx = dx * 0.5f;
		float hy = dy * 0.5f;
		
		x += hx;
		y += hy;
		
		width  -= hx;
		height -= hy;
	}
	
	
	RN_INLINE float Rect::Top() const
	{
		return y;
	}
	
	RN_INLINE float Rect::Bottom() const
	{
		return y + height;
	}
	
	RN_INLINE float Rect::Left() const
	{
		return x;
	}
	
	RN_INLINE float Rect::Right() const
	{
		return x + width;
	}
}

#endif /* __RAYNE_RECT_H__ */
