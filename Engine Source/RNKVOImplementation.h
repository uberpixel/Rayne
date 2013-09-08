//
//  RNKVOImplementation.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KVOIMPLEMENTATION
#define __RAYNE_KVOIMPLEMENTATION

#include "RNBase.h"
#include "RNKVO.h"
#include "RNObject.h"
#include "RNNumber.h"

namespace RN
{
	template<class T>
	class __ObservableBase
	{};
	
	template<class T>
	class Observable
	{};

#define __ObservableScalar(type, kvotype) \
	template<> \
	class __ObservableBase<type> : public ObservableVariable<type> \
	{ \
	public: \
		__ObservableBase(type *ptr, const char *name) : \
			ObservableVariable(ptr, name, ObservableType::kvotype) \
		{} \
		void SetValue(Object *value) override \
		{ \
			RN_ASSERT(value->IsKindOfClass(Number::MetaClass()), ""); \
			Number *number = static_cast<Number *>(value); \
			WillChangeValue(); \
			*_value = number->Get##kvotype##Value(); \
			DidChangeValue();  \
		} \
		Object *GetValue() const override \
		{ \
			return Number::With##kvotype (*_value); \
		} \
	}; \
	template<> \
	class Observable<type> : public __ObservableBase<type> \
	{ \
	public: \
		Observable(const char *name) : \
			__ObservableBase(&_storage, name) \
		{} \
		Observable(const char *name, const type& initial) : \
			__ObservableBase(&_storage, name), \
			_storage(initial) \
		{} \
		bool operator == (const type& other) const \
		{ \
			return (_storage == other); \
		} \
		bool operator != (const type& other) const \
		{ \
			return (_storage != other); \
		} \
		 \
		operator type& () \
		{ \
			return _storage; \
		} \
		operator type() const \
		{ \
			return _storage; \
		} \
		 \
		type& operator= (const type& other) \
		{ \
			WillChangeValue(); \
			_storage = other; \
			DidChangeValue(); \
			 \
			return _storage; \
		} \
		\
		type& operator+= (const type& other) \
		{ \
			WillChangeValue(); \
			_storage += other; \
			DidChangeValue(); \
			 \
			return _storage; \
		} \
		type& operator-= (const type& other) \
		{ \
			WillChangeValue(); \
			_storage -= other; \
			DidChangeValue(); \
			 \
			return _storage; \
		} \
		type& operator*= (const type& other) \
		{ \
			WillChangeValue(); \
			_storage *= other; \
			DidChangeValue(); \
			 \
			return _storage; \
		} \
		type& operator/= (const type& other) \
		{ \
			WillChangeValue(); \
			_storage /= other; \
			DidChangeValue(); \
			 \
			return _storage; \
		} \
		 \
		type operator+ (const type& other) const \
		{ \
			return _storage + other; \
		} \
		type operator- (const type& other) const \
		{ \
			return _storage - other; \
		} \
		type operator* (const type& other) const \
		{ \
			return _storage * other; \
		} \
		type operator/ (const type& other) const \
		{ \
			return _storage / other; \
		} \
		 \
		type& operator ++() \
		{ \
			WillChangeValue(); \
			++ _storage; \
			DidChangeValue(); \
			 \
			return _storage; \
		} \
		type operator ++(int) \
		{ \
			type result = _storage; \
			 \
			WillChangeValue(); \
			++ _storage; \
			DidChangeValue(); \
			 \
			return result; \
		} \
		type& operator --() \
		{ \
			WillChangeValue(); \
			-- _storage; \
			DidChangeValue(); \
			 \
			return _storage; \
		} \
		type operator --(int) \
		{ \
			type result = _storage; \
			 \
			WillChangeValue(); \
			-- _storage; \
			DidChangeValue(); \
			 \
			return result; \
		} \
	private: \
		type _storage; \
	};

	__ObservableScalar(int8, Int8)
	__ObservableScalar(int16, Int16)
	__ObservableScalar(int32, Int32)
	__ObservableScalar(int64, Int64)
	
	__ObservableScalar(uint8, Uint8)
	__ObservableScalar(uint16, Uint16)
	__ObservableScalar(uint32, Uint32)
	__ObservableScalar(uint64, Uint64)
	
	__ObservableScalar(float, Float)
	__ObservableScalar(double, Double)
}

#endif /* __RAYNE_KVOIMPLEMENTATION */
