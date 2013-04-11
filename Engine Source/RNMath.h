//
//  RNMath.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MATH_H__
#define __RAYNE_MATH_H__

#include "RNSIMD.h"

#define kRNEpsilonFloat 0.001f

#ifndef MAX
	#define MAX(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#endif

#ifndef MIN
	#define MIN(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _b : _a; })
#endif

#define kRNE         2.71828182845904523536028747135266250f
#define kRNLOG2E     1.44269504088896340735992468100189214f
#define kRNLOG10E    0.434294481903251827651128918916605082f
#define kRNLN2       0.693147180559945309417232121458176568f
#define kRNLN10      2.30258509299404568401799145468436421f
#define kRNPI        3.14159265358979323846264338327950288f
#define kRNPI_2      1.57079632679489661923132169163975144f
#define kRNPI_4      0.785398163397448309615660845819875721f
#define kRN1_PI      0.318309886183790671537767526745028724f
#define kRN2_PI      0.636619772367581343075535053490057448f
#define kRN2_SQRTPI  1.12837916709551257389615890312154517f
#define kRNSQRT2     1.41421356237309504880168872420969808f
#define kRNSQRT1_2   0.707106781186547524400844362104849039f

namespace RN
{
	namespace Math
	{
		static inline float FastAbs(float val)
		{
			union
			{
				float fval;
				int ival;
			} tval;
			
			tval.fval = val;
			tval.ival &= (1 << 31) - 1;
			
			return tval.fval;
		}
		
		static inline double FastAbs(double val)
		{
			union
			{
				double dval;
				long long ival;
			} tval;
			
			tval.dval = val;
			tval.ival &= (1LL << 63) - 1;
			
			return tval.dval;
		}
		
		static inline bool IsNegative(float val)
		{
			union
			{
				float fval;
				int ival;
			} tval;
			
			tval.fval = val;
			return (tval.ival & (1 << 31));
		}
		
		static inline bool IsNegative(double val)
		{
			union
			{
				double dval;
				long long ival;
			} tval;
			
			tval.dval = val;
			return (tval.ival & (1LL << 63));
		}
		
		
		float Sqrt(float x);
		float InverseSqrt(float x);
		
		float Sin(float x);
		float Cos(float x);
	}
}

#endif
