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
	class __ObservableOperatorsArithmetic : public virtual ObservableVariable<T>
	{
		/*T& operator= (const T& other)
		{
			WillChangeValue();
			*_value = other;
			DidChangeValue();
			
			return *_value;
		}
		
		T& operator+= (const T& other)
		{
			WillChangeValue();
			*_value += other;
			DidChangeValue();
			
			return *_value;
		}
		
		T& operator-= (const T& other)
		{
			WillChangeValue();
			*_value -= other;
			DidChangeValue();
			
			return *_value;
		}
		
		T& operator*= (const T& other)
		{
			WillChangeValue();
			*_value *= other;
			DidChangeValue();
			
			return *_value;
		}
		
		T& operator/= (const T& other)
		{
			WillChangeValue();
			*_value /= other;
			DidChangeValue();
			
			return *_value;
		}
		
		
		T operator+ (const T& other) const
		{
			return *_value + other;
		}
		
		T operator- (const T& other) const
		{
			return *_value - other;
		}
		
		T operator* (const T& other) const
		{
			return *_value * other;
		}
		
		T operator/ (const T& other) const
		{
			return *_value / other;
		}*/
	};

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
		type& operator= (type other) \
		{ \
			WillChangeValue(); \
			_storage = other; \
			DidChangeValue(); \
			return _storage; \
		} \
	private: \
		type _storage; \
	};

	__ObservableScalar(int32, Int32)
	__ObservableScalar(float, Float)
}

#endif /* __RAYNE_KVOIMPLEMENTATION */
