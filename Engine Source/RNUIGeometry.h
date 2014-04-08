//
//  RNUIGeometry.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIGEOMETRY_H__
#define __RAYNE_UIGEOMETRY_H__

#include "RNBase.h"
#include "RNVector.h"
#include "RNMath.h"
#include "RNRect.h"

namespace RN
{
	namespace UI
	{
		struct EdgeInsets
		{
			EdgeInsets()
			{
				top = bottom = left = right = 0.0f;
			}
			
			EdgeInsets(float ttop, float tbottom, float tleft, float tright)
			{
				top = ttop;
				bottom = tbottom;
				left = tleft;
				right = tright;
			}
			
			bool operator ==(const EdgeInsets& other)
			{
				if(Math::FastAbs(top - other.top) >= k::EpsilonFloat)
					return false;
				
				if(Math::FastAbs(bottom - other.bottom) >= k::EpsilonFloat)
					return false;
				
				if(Math::FastAbs(left - other.left) >= k::EpsilonFloat)
					return false;
				
				if(Math::FastAbs(right - other.right) >= k::EpsilonFloat)
					return false;
				
				return true;
			}
			
			bool operator !=(const EdgeInsets& other)
			{
				return !(*this == other);
			}
			
			float top, bottom, left, right;
		};
		
		struct Atlas
		{
			Atlas(float tu1, float tv1, float tu2, float tv2)
			{
				u1 = tu1;
				v1 = tv1;
				
				u2 = tu2;
				v2 = tv2;
			}
			
			Atlas(const Vector2& uv1, const Vector2& uv2)
			{
				u1 = uv1.x;
				v1 = uv1.y;
				
				u2 = uv2.x;
				v2 = uv2.y;
			}
			
			Atlas(const Rect &rect)
			{
				u1 = rect.x;
				v1 = rect.y;
				
				u2 = u1 + rect.width;
				v2 = v1 + rect.height;
			}
			
			float u1, v1, u2, v2;
		};
		
		enum class ScaleMode
		{
			None,
			ProportionallyDown,
			ProportionallyUpOrDown,
			AxisIndependently
		};
		
		enum class ImagePosition
		{
			NoImage,
			ImageOnly,
			Left,
			Right,
			Below,
			Above,
			Overlaps
		};
	}
}

#endif /* __RAYNE_UIGEOMETRY_H__ */
