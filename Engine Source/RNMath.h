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
#include "RNDefines.h"
#include "RNConstants.h"

#ifndef MAX
	#define MAX(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#endif

#ifndef MIN
	#define MIN(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _b : _a; })
#endif

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
		
		static inline bool Compare(float x, float y, float delta = k::EpsilonFloat)
		{
			return (FastAbs(x - y) < delta);
		}
		
		static inline bool Compare(double x, double y, float delta = k::EpsilonFloat)
		{
			return (FastAbs(x - y) < delta);
		}
		
		static inline float RadiansToDegrees(float radians)
		{
			return radians * 57.2957795130823208768f;
		}
		
		static inline float DegreesToRadians(float degrees)
		{
			return degrees * 0.01745329251994329577f;
		}
		
		
		RNAPI float Sqrt(float x);
		RNAPI float InverseSqrt(float x);
		
		RNAPI float Sin(float x);
		RNAPI float Cos(float x);
	}
}

#endif
