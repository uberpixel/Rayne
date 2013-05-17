//
//  RNConstants.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CONSTANTS_H__
#define __RAYNE_CONSTANTS_H__

#include <cfloat>
#include "RNDefines.h"

namespace RN
{
	namespace k
	{
		static const float Pi   = 3.14159265358979323846264338327950288f;
		static const float Pi_2 = 1.57079632679489661923132169163975144f;
		static const float Pi_4 = 0.785398163397448309615660845819875721f;
		
		static const float E      = 2.71828182845904523536028747135266250f;
		static const float Log2E  = 1.44269504088896340735992468100189214f;
		static const float Log10E = 0.434294481903251827651128918916605082f;
		
		static const float Sqrt2   = 1.41421356237309504880168872420969808f;
		static const float Sqrt1_2 = 0.707106781186547524400844362104849039f;
		
		static const float EpsilonFloat = FLT_EPSILON;
		
		static const machine_uint NotFound = static_cast<machine_uint>(-1);
	}
}

#define kRNNotFound RN::k::NotFound

#endif /* __RAYNE_CONSTANTS_H__ */
