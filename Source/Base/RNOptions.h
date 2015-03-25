//
//  RNOptions.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPTIONS_H__
#define __RAYNE_OPTIONS_H__

#include <type_traits>

namespace RN
{
	template<class T, bool>
	struct __Options;

	template<class T>
	struct __Options<T, true>
	{
		static_assert(std::is_integral<T>::value, "T must be an integer value");

		__Options() = default;

		T operator =(T value)
		{
			_value = value;
			return value;
		}
		operator T() const
		{
			return _value;
		}


		T operator ~() const
		{
			return ~_value;
		}


		T operator &(T value) const
		{
			return _value & value;
		}
		T operator |(T value) const
		{
			return _value | value;
		}
		T &operator &=(T value)
		{
			return _value &= value;
		}
		T &operator |=(T value)
		{
			return _value |= value;
		}

	protected:
		T _value;
	};

	template<class T>
	struct __Options<T, false>
	{
		static_assert(std::is_integral<T>::value, "T must be an integer value");

		__Options() = default;

		T operator =(T value)
		{
			_value = value;
			return value;
		}
		operator T() const
		{
			return _value;
		}


		T operator ~() const
		{
			return ~_value;
		}


		T operator &(T value) const
		{
			return _value & value;
		}
		T operator |(T value) const
		{
			return _value | value;
		}
		T &operator &=(T value)
		{
			return _value &= value;
		}
		T &operator |=(T value)
		{
			return _value |= value;
		}


		T operator &(int value) const
		{
			return _value & value;
		}
		T operator |(int value) const
		{
			return _value | value;
		}

		T &operator &=(int value)
		{
			return _value &= value;
		}
		T &operator |=(int value)
		{
			return _value |= value;
		}

	protected:
		T _value;
	};

	template<class T = uint32>
	using Options = __Options<T, std::is_same<T, int>::value>;


#define RN_OPTIONS(name, type, ...) \
	struct name : public RN::Options<type> \
    { \
        name() = default; \
        name(int value) { _value = value; } \
        enum { \
            __VA_ARGS__ \
        }; \
	}
}

#endif /* __RAYNE_OPTIONS_H__ */
