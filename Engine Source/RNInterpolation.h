//
//  RNInterpolation.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INTERPOLATION_H__
#define __RAYNE_INTERPOLATION_H__

#include "RNBase.h"

namespace RN
{
	typedef enum
	{
		InterpolationTypeLinear,
		InterpolationTypeQuadraticEaseIn,
		InterpolationTypeQuadraticEaseOut,
		InterpolationTypeQuadraticEaseInOut,
		InterpolationTypeSinusoidalEaseIn,
		InterpolationTypeSinusoidalEaseOut,
		InterpolationTypeSinusoidalEaseInOut,
		InterpolationTypeExponentialEaseIn,
		InterpolationTypeExponentialEaseOut,
		InterpolationTypeExponentialEaseInOut,
		InterpolationTypeCircularEaseIn,
		InterpolationTypeCircularEaseOut,
		InterpolationTypeCircularEaseInOut,
		InterpolationTypeCubicEaseIn,
		InterpolationTypeCubicEaseOut,
		InterpolationTypeCubicEaseInOut,
		InterpolationTypeQuarticEaseIn,
		InterpolationTypeQuarticEaseOut,
		InterpolationTypeQuarticEaseInOut,
		InterpolationTypeQuinticEaseIn,
		InterpolationTypeQuinticEaseOut,
		InterpolationTypeQuinticEaseInOut
	} InterpolationType;
	
	template <typename T, typename TimeType=float>
	class Interpolator
	{
	public:
		Interpolator(T start, T end, TimeType duration, InterpolationType type=InterpolationTypeLinear)
		{
			_startValue = start;
			_endValue   = end;
			_type       = type;
			
			_duration = duration;
			_difference   = (_endValue - _startValue);
		}
		
		static T ByValueAtPoint(T start, T end, TimeType time, TimeType duration, InterpolationType type=InterpolationTypeLinear)
		{
			Interpolator temp(start, end, duration, type);
			return temp.ByValue(time);
		}
		
		T ByValue(TimeType time) const
		{
			switch(_type)
			{
				case InterpolationTypeLinear:
					return _startValue + (_difference * time / _duration);
					break;
					
				// Quadratic easing
				case InterpolationTypeQuadraticEaseIn:
				{
					TimeType t = time / _duration;
					return _startValue + (_difference * t * t);
				}
				
				case InterpolationTypeQuadraticEaseOut:
				{
					TimeType t = time / _duration;
					return _startValue + (-_difference * t * (t - 2.0));
				}
					
				case InterpolationTypeQuadraticEaseInOut:
				{
					TimeType t = time / (_duration / 2.0);
					if(t < 1.0)
						return _startValue + (_difference / 2.0 * t * t);
					
					t -= 1.0;
					return _startValue + (-_difference / 2.0 * (t * (t - 2.0) - 1.0));
				}
					
				// Sinusoidal easing
				case InterpolationTypeSinusoidalEaseIn:
					return _startValue + (-_difference * Math::Cos(time / _duration * kRNPI_2) + _difference);
					break;
				
				case InterpolationTypeSinusoidalEaseOut:
					return _startValue + (_difference * Math::Sin(time / _duration * kRNPI_2));
					break;
					
				case InterpolationTypeSinusoidalEaseInOut:
					return _startValue + (-_difference / 2.0 * (Math::Cos(kRNPI * time / _duration) - 1.0));
					break;
					
				// Exponential easing
				case InterpolationTypeExponentialEaseIn:
					return _startValue + (_difference * pow(2.0, 10 * (time / _duration - 1.0)));
					break;
					
				case InterpolationTypeExponentialEaseOut:
					return _startValue + (_difference * (-pow(2.0, - 10.0 * time / _duration) + 1.0));
					break;
					
				case InterpolationTypeExponentialEaseInOut:
				{
					TimeType t = time / (_duration / 2.0);
					if(t < 1.0)
						return _startValue + (_difference / 2.0 * pow(2.0, 10.0 * (t - 1.0)));
					
					t -= 1.0;
					return _startValue + (_difference / 2.0 * (-pow(2.0, -10 * t) + 2.0));
				}
					
				// Circular easing
				case InterpolationTypeCircularEaseIn:
				{
					TimeType t = time / _duration;
					return _startValue + (-_difference * (Math::Sqrt(1.0 - t * t) - 1.0));
				}
					
				case InterpolationTypeCircularEaseOut:
				{
					TimeType t = (time / _duration) - 1.0;
					return _startValue + (_difference * Math::Sqrt(1.0 - t * t));
				}
					
				case InterpolationTypeCircularEaseInOut:
				{
					TimeType t = time / (_duration / 2.0);
					if(t < 1.0)
						return _startValue + (-_difference / 2.0 * (Math::Sqrt(1.0 - t * t) - 1.0));
					
					t -= 2.0;
					return _startValue + (_difference / 2.0 * (Math::Sqrt(1.0 - t * t) + 1.0));
				}
				
				// Cubic easing
				case InterpolationTypeCubicEaseIn:
				{
					TimeType t = time / _duration;
					return _startValue + (_difference * t * t * t);
				}
					
				case InterpolationTypeCubicEaseOut:
				{
					TimeType t = (time / _duration) - 1.0;
					return _startValue + (_difference * (t * t * t + 1.0));
				}
					
				case InterpolationTypeCubicEaseInOut:
				{
					TimeType t = time / (_duration / 2.0);
					if(t < 1.0)
						return _startValue + (_difference / 2.0 * t * t * t);
					
					t -= 2.0;
					return _startValue + (_difference / 2.0 * (t * t * t + 2.0));
				}
					
				// Quartic easing
				case InterpolationTypeQuarticEaseIn:
				{
					TimeType t = time / _duration;
					return _startValue + (_difference * t * t * t * t);
				}
					
				case InterpolationTypeQuarticEaseOut:
				{
					TimeType t = (time / _duration) - 1.0;
					return _startValue + (-_difference * (t * t * t * t - 1.0));
				}
					
				case InterpolationTypeQuarticEaseInOut:
				{
					TimeType t = time / (_duration / 2.0);
					if(t < 1.0)
						return _startValue + (_difference / 2.0 * t * t * t * t);
					
					t -= 2.0;
					return _startValue + (-_difference / 2.0 * (t * t * t * t - 2.0));
				}
					
				// Quintic easing
				case InterpolationTypeQuinticEaseIn:
				{
					TimeType t = time / _duration;
					return _startValue + (_difference * t * t * t * t * t);
				}
					
				case InterpolationTypeQuinticEaseOut:
				{
					TimeType t = (time / _duration) - 1.0;
					return _startValue + (_difference * (t * t * t * t * t + 1.0));
				}
					
				case InterpolationTypeQuinticEaseInOut:
				{
					TimeType t = time / (_duration / 2.0);
					if(t < 1.0)
						return _startValue + (_difference / 2.0 * t * t * t * t * t);
					
					t -= 2.0;
					return _startValue + (_difference / 2.0 * (t * t * t * t * t + 2.0));
				}
			}
		}
			
		void Reverse()
		{
			std::swap(_startValue, _endValue);
			_difference = (_endValue - _startValue);
		}
		
		T StartValue() const
		{
			return _startValue;
		}
		
		T EndValue() const
		{
			return _endValue;
		}
		
		InterpolationType Type() const
		{
			return _type;
		}
		
	private:
		InterpolationType _type;
		TimeType _duration;
		
		T _startValue;
		T _endValue;
		T _difference;
	};
}

#endif /* __RAYNE_INTERPOLATION_H__ */
