//
//  RNEnum.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ENUM_H__
#define __RAYNE_ENUM_H__

namespace RN
{
	template<class T, bool overrideBitSetOperators>
	struct __EnumBase;
	
	template<class T>
	struct __EnumBase<T, true>
	{
		T operator ~()
		{
			return ~_value;
		}
		T operator &(int value)
		{
			return _value & value;
		}
		T operator |(int value)
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
	
	template<class T>
	struct __EnumBase<T, false>
	{
	protected:
		T _value;
	};
	
	
	
	template<class T, bool overrideBitSetOperators = true>
	struct Enum : public __EnumBase<T, overrideBitSetOperators>
	{
		using __EnumBase<T, overrideBitSetOperators>::_value;
		
		Enum()
		{
			_value = 0;
		}
		Enum(T value)
		{
			_value = value;
		}
		
		T operator =(T value)
		{
			_value = value;
			return value;
		}
		operator T()
		{
			return _value;
		}
	};
}

#endif /* __RAYNE_ENUM_H__ */
