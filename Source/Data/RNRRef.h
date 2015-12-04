//
//  RNRef.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RREF_H__
#define __RAYNE_RREF_H__

#include "../Base/RNBase.h"

namespace RN
{
	template<class T>
	class RRef
	{
	public:
		RRef(T &&value) :
			_value(std::move(value))
		{}
		RRef(RRef &other) :
			_value(std::move(other._value))
		{}
		RRef(RRef &&other) :
			_value(std::move(other._value))
		{}

		RRef() = delete;

		RRef &operator =(RRef other)
		{
			_value = std::move(other._value);
		}

		T &&Move()
		{
			return std::move(_value);
		}

	private:
		T _value;
	};

	template<class T>
	RRef<T> MakeRRef(T &&val)
	{
		return RRef<T>(std::move(val));
	}
};

#endif /* __RAYNE_RREF_H__ */
