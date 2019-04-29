//
//  RNInterpolation.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INTERPOLATION_H__
#define __RAYNE_INTERPOLATION_H__

#include "../Base/RNBase.h"

namespace RN
{
	template <typename T, typename TimeType=float>
	class Interpolator
	{
	public:
		enum class Type
		{
			Linear,
			QuadraticEaseIn,
			QuadraticEaseOut,
			QuadraticEaseInOut,
			SinusoidalEaseIn,
			SinusoidalEaseOut,
			SinusoidalEaseInOut,
			ExponentialEaseIn,
			ExponentialEaseOut,
			ExponentialEaseInOut,
			CircularEaseIn,
			CircularEaseOut,
			CircularEaseInOut,
			CubicEaseIn,
			CubicEaseOut,
			CubicEaseInOut,
			QuarticEaseIn,
			QuarticEaseOut,
			QuarticEaseInOut,
			QuinticEaseIn,
			QuinticEaseOut,
			QuinticEaseInOut
		};
		
		Interpolator(Type type = Type::Linear)
		{
			_type = type;
		}
		
		Interpolator(T start, T end, TimeType duration, Type type = Type::Linear)
		{
			_startValue = start;
			_endValue   = end;
			_type       = type;
			
			_duration = duration;
			_difference   = (_endValue - _startValue);
		}
		
		void SetStartValue(const T &value)
		{
			_startValue = value;
			_difference = (_endValue - _startValue);
		}
		
		void SetEndValue(const T &value)
		{
			_endValue = value;
			_difference = (_endValue - _startValue);
		}
		
		void SetValues(const T &start, const T &end)
		{
			_startValue = start;
			_endValue = end;
			_difference = (_endValue - _startValue);
		}
		
		void SetDuration(TimeType duration)
		{
			_duration = duration;
		}
		
		void SetType(Type type)
		{
			_type = type;
		}
		
		
		static T GetValueAtPoint(T start, T end, TimeType time, TimeType duration, Type type=Type::Linear)
		{
			Interpolator temp(start, end, duration, type);
			return temp.GetValue(time);
		}
		
		T GetValue(TimeType time) const
		{
			switch(_type)
			{
				case Type::Linear:
					return _startValue + (_difference * time / _duration);
					break;
					
				// Quadratic easing
				case Type::QuadraticEaseIn:
				{
					TimeType t = time / _duration;
					return _startValue + (_difference * t * t);
				}
				
				case Type::QuadraticEaseOut:
				{
					TimeType t = time / _duration;
					return _startValue + (-_difference * t * (t - 2.0));
				}
					
				case Type::QuadraticEaseInOut:
				{
					TimeType t = time / (_duration / 2.0);
					if(t < 1.0)
						return _startValue + (_difference / 2.0 * t * t);
					
					t -= 1.0;
					return _startValue + (-_difference / 2.0 * (t * (t - 2.0) - 1.0));
				}
					
				// Sinusoidal easing
				case Type::SinusoidalEaseIn:
					return _startValue + (-_difference * Math::Cos(time / _duration * k::Pi_2) + _difference);
					break;
				
				case Type::SinusoidalEaseOut:
					return _startValue + (_difference * Math::Sin(time / _duration * k::Pi_2));
					break;
					
				case Type::SinusoidalEaseInOut:
					return _startValue + (-_difference / 2.0 * (Math::Cos(k::Pi * time / _duration) - 1.0));
					break;
					
				// Exponential easing
				case Type::ExponentialEaseIn:
					return _startValue + (_difference * pow(2.0, 10 * (time / _duration - 1.0)));
					break;
					
				case Type::ExponentialEaseOut:
					return _startValue + (_difference * (-pow(2.0, - 10.0 * time / _duration) + 1.0));
					break;
					
				case Type::ExponentialEaseInOut:
				{
					TimeType t = time / (_duration / 2.0);
					if(t < 1.0)
						return _startValue + (_difference / 2.0 * pow(2.0, 10.0 * (t - 1.0)));
					
					t -= 1.0;
					return _startValue + (_difference / 2.0 * (-pow(2.0, -10 * t) + 2.0));
				}
					
				// Circular easing
				case Type::CircularEaseIn:
				{
					TimeType t = time / _duration;
					return _startValue + (-_difference * (Math::Sqrt(1.0 - t * t) - 1.0));
				}
					
				case Type::CircularEaseOut:
				{
					TimeType t = (time / _duration) - 1.0;
					return _startValue + (_difference * Math::Sqrt(1.0 - t * t));
				}
					
				case Type::CircularEaseInOut:
				{
					TimeType t = time / (_duration / 2.0);
					if(t < 1.0)
						return _startValue + (-_difference / 2.0 * (Math::Sqrt(1.0 - t * t) - 1.0));
					
					t -= 2.0;
					return _startValue + (_difference / 2.0 * (Math::Sqrt(1.0 - t * t) + 1.0));
				}
				
				// Cubic easing
				case Type::CubicEaseIn:
				{
					TimeType t = time / _duration;
					return _startValue + (_difference * t * t * t);
				}
					
				case Type::CubicEaseOut:
				{
					TimeType t = (time / _duration) - 1.0;
					return _startValue + (_difference * (t * t * t + 1.0));
				}
					
				case Type::CubicEaseInOut:
				{
					TimeType t = time / (_duration / 2.0);
					if(t < 1.0)
						return _startValue + (_difference / 2.0 * t * t * t);
					
					t -= 2.0;
					return _startValue + (_difference / 2.0 * (t * t * t + 2.0));
				}
					
				// Quartic easing
				case Type::QuarticEaseIn:
				{
					TimeType t = time / _duration;
					return _startValue + (_difference * t * t * t * t);
				}
					
				case Type::QuarticEaseOut:
				{
					TimeType t = (time / _duration) - 1.0;
					return _startValue + (-_difference * (t * t * t * t - 1.0));
				}
					
				case Type::QuarticEaseInOut:
				{
					TimeType t = time / (_duration / 2.0);
					if(t < 1.0)
						return _startValue + (_difference / 2.0 * t * t * t * t);
					
					t -= 2.0;
					return _startValue + (-_difference / 2.0 * (t * t * t * t - 2.0));
				}
					
				// Quintic easing
				case Type::QuinticEaseIn:
				{
					TimeType t = time / _duration;
					return _startValue + (_difference * t * t * t * t * t);
				}
					
				case Type::QuinticEaseOut:
				{
					TimeType t = (time / _duration) - 1.0;
					return _startValue + (_difference * (t * t * t * t * t + 1.0));
				}
					
				case Type::QuinticEaseInOut:
				{
					TimeType t = time / (_duration / 2.0);
					if(t < 1.0)
						return _startValue + (_difference / 2.0 * t * t * t * t * t);
					
					t -= 2.0;
					return _startValue + (_difference / 2.0 * (t * t * t * t * t + 2.0));
				}

				default:
					RN_ASSERT(false, "This should NEVER be reached!");
					break;
			}
		}
			
		void Reverse()
		{
			std::swap(_startValue, _endValue);
			_difference = (_endValue - _startValue);
		}
		
		const T &GetStartValue() const
		{
			return _startValue;
		}
		
		const T &GetEndValue() const
		{
			return _endValue;
		}
		
		Type GetType() const
		{
			return _type;
		}
		
	private:
		Type _type;
		TimeType _duration;
		
		T _startValue;
		T _endValue;
		T _difference;
	};
}

#endif /* __RAYNE_INTERPOLATION_H__ */
