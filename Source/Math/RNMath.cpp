//
//  RNMath.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMath.h"
#include "RNVector.h"

namespace RN
{
	namespace Math
	{
		float Sqrt(float x)
		{
			return sqrtf(x);
		}
		
		float InverseSqrt(float x)
		{
			return 1.0 / sqrtf(x);
		}
		
		
		float Sin(float x)
		{
			return sinf(x);
		}
		
		float Cos(float x)
		{
			return cosf(x);
		}
	
		//Taken from https://developer.android.google.cn/games/optimize/vertex-data-management
		uint16 ConvertFloatToHalf(float value)
		{
			uint32 x = *(uint32 *)&value;
			uint32 sign = (uint16)(x >> 31);
			uint32 mantissa;
			uint32 exp;
			uint16 hf;

			mantissa = x & ((1 << 23) - 1);
			exp = x & (0xFF << 23);
			if(exp >= 0x47800000)
			{
				// check if the original number is a NaN
				if(mantissa && (exp == (0xFF << 23)))
				{
					// single precision NaN
					mantissa = (1 << 23) - 1;
				}
				else
				{
					// half-float will be Inf
					mantissa = 0;
				}
				hf = (((uint16)sign) << 15) | (uint16)(0x1F << 10) | (uint16)(mantissa >> 13);
			}
			// check if exponent is <= -15
			else if(exp <= 0x38000000)
			{
				hf = 0;  // too small to be represented
			}
			else
			{
				hf = (((uint16)sign) << 15) | (uint16)((exp - 0x38000000) >> 13) | (uint16)(mantissa >> 13);
			}

			return hf;
		}
	
		//Taken from: https://gist.github.com/milhidaka/95863906fe828198f47991c813dbe233
		float ConvertHalfToFloat(uint16 value)
		{
		  // MSB -> LSB
		  // float16=1bit: sign, 5bit: exponent, 10bit: fraction
		  // float32=1bit: sign, 8bit: exponent, 23bit: fraction
		  // for normal exponent(1 to 0x1e): value=2**(exponent-15)*(1.fraction)
		  // for denormalized exponent(0): value=2**-14*(0.fraction)
		  uint32_t sign = value >> 15;
		  uint32_t exponent = (value >> 10) & 0x1F;
		  uint32_t fraction = (value & 0x3FF);
		  uint32_t float32_value;
		  if (exponent == 0)
		  {
			if (fraction == 0)
			{
			  // zero
			  float32_value = (sign << 31);
			}
			else
			{
			  // can be represented as ordinary value in float32
			  // 2 ** -14 * 0.0101
			  // => 2 ** -16 * 1.0100
			  // int int_exponent = -14;
			  exponent = 127 - 14;
			  while ((fraction & (1 << 10)) == 0)
			  {
				//int_exponent--;
				exponent--;
				fraction <<= 1;
			  }
			  fraction &= 0x3FF;
			  // int_exponent += 127;
			  float32_value = (sign << 31) | (exponent << 23) | (fraction << 13);
			}
		  }
		  else if (exponent == 0x1F)
		  {
			/* Inf or NaN */
			float32_value = (sign << 31) | (0xFF << 23) | (fraction << 13);
		  }
		  else
		  {
			/* ordinary number */
			float32_value = (sign << 31) | ((exponent + (127-15)) << 23) | (fraction << 13);
		  }
		  
		  return *((float*)&float32_value);
		}
	}
}
