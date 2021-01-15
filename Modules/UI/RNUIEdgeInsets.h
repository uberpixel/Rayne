//
//  RNUIEdgeInsets.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIEDGEINSETS_H_
#define __RAYNE_UIEDGEINSETS_H_

#include "RNUIConfig.h"

namespace RN
{
	struct EdgeInsets
	{
		EdgeInsets() :
			top(0.0f),
			bottom(0.0f),
			left(0.0f),
			right(0.0f)
		{}

		EdgeInsets(float ttop, float tbottom, float tleft, float tright) :
			top(ttop),
			bottom(tbottom),
			left(tleft),
			right(tright)
		{}

		bool operator ==(const EdgeInsets &other)
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

		bool operator !=(const EdgeInsets &other)
		{
			return !(*this == other);
		}

		float top, bottom, left, right;
	};

}


#endif /* __RAYNE_UIEDGEINSETS_H_ */
