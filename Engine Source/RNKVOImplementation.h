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
#include "RNValue.h"

namespace RN
{
	template<class T>
	class ObservableScalar
	{};
	
	template<class T>
	class ObservableValue
	{};
	
	template<class T>
	class Observable
	{};

#define __ObservableScalar(type, kvotype) \
	template<> \
	class ObservableScalar<type> : public ObservableBase \
	{ \
	public: \
		typedef std::function<void (type)> SetterCallback; \
		typedef std::function<type (void)> GetterCallback; \
		ObservableScalar(const char *name, GetterCallback getter, SetterCallback setter) : \
			ObservableBase(name, ObservableType::kvotype), \
			_getter(getter), \
			_setter(setter) \
		{ \
			SetWritable(static_cast<bool>(_setter)); \
		} \
		void SetValue(Object *value) override \
		{ \
			RN_ASSERT(value->IsKindOfClass(Number::MetaClass()), ""); \
			Number *number = static_cast<Number *>(value); \
			WillChangeValue(); \
			_setter(number->Get##kvotype##Value()); \
			DidChangeValue();  \
		} \
		Object *GetValue() const override \
		{ \
			return Number::With##kvotype (_getter()); \
		} \
	protected: \
		SetterCallback _setter; \
		GetterCallback _getter; \
	}; \
	template<> \
	class Observable<type> : public ObservableScalar<type> \
	{ \
	public: \
		Observable(const char *name, GetterCallback getter, SetterCallback setter = SetterCallback()) : \
			ObservableScalar(name, getter, setter) \
		{} \
		Observable(const char *name, const type& initial, GetterCallback getter, SetterCallback setter = SetterCallback()) : \
			ObservableScalar(name, getter, setter), \
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

	
#define __ObservableValueBegin(type) \
	template<> \
	class ObservableValue<type> : public ObservableBase \
	{ \
	public: \
		typedef std::function<void (const type&)> SetterCallback; \
		typedef std::function<type (void)> GetterCallback; \
		ObservableValue(const char *name, GetterCallback getter, SetterCallback setter) : \
			ObservableBase(name, ObservableType::type), \
			_getter(getter), \
			_setter(setter) \
		{ \
			SetWritable(static_cast<bool>(_setter)); \
		} \
		void SetValue(Object *tvalue) override \
		{ \
			RN_ASSERT(tvalue->IsKindOfClass(Value::MetaClass()), ""); \
			Value *value = static_cast<Value *>(tvalue); \
			WillChangeValue(); \
			_setter(value->GetValue<type>()); \
			DidChangeValue();  \
		} \
		Object *GetValue() const override \
		{ \
			return Value::With##type (_getter()); \
		} \
	protected: \
		SetterCallback _setter; \
		GetterCallback _getter; \
	}; \
	template<> \
	class Observable<type> : public ObservableValue<type> \
	{ \
	public: \
		Observable(const char *name, GetterCallback getter, SetterCallback setter = SetterCallback()) : \
			ObservableValue(name, getter, setter) \
		{} \
		Observable(const char *name, const type& initial, GetterCallback getter, SetterCallback setter = SetterCallback()) : \
			ObservableValue(name, getter, setter), \
			_storage(initial) \
		{} \
		\
		operator type& () \
		{ \
			return _storage; \
		} \
		operator const type& () const \
		{ \
			return _storage; \
		} \
		type& operator= (const type& other) \
		{ \
			WillChangeValue(); \
			_storage = other; \
			DidChangeValue(); \
			\
			return _storage; \
		} \
	private: \
		type _storage; \
	public:
	

#define __ObservableValueComparison(type) \
	bool operator == (const type& other) const \
	{ \
		return (_storage == other); \
	} \
	bool operator != (const type& other) const \
	{ \
		return (_storage != other); \
	} \

#define __ObservableValuePointerLikes(type) \
	type& operator *() \
	{ \
		return _storage; \
	} \
	const type& operator *() const \
	{ \
		return _storage; \
	} \
	type *operator ->() \
	{ \
		return &_storage; \
	} \
	const type *operator ->() const \
	{ \
		return &_storage; \
	}

#define __ObservableValueBinaryArithmeticAddition(type) \
	type& operator+= (const type& other) \
	{ \
		WillChangeValue(); \
		_storage += other; \
		DidChangeValue(); \
		\
		return _storage; \
	} \
	type operator+ (const type& other) const \
	{ \
		return _storage + other; \
	}

#define __ObservableValueBinaryArithmeticSubtraction(type) \
	type& operator-= (const type& other) \
	{ \
		WillChangeValue(); \
		_storage -= other; \
		DidChangeValue(); \
		\
		return _storage; \
	} \
	type operator- (const type& other) const \
	{ \
		return _storage - other; \
	}

#define __ObservableValueBinaryArithmeticMultiplication(type) \
	type& operator*= (const type& other) \
	{ \
		WillChangeValue(); \
		_storage *= other; \
		DidChangeValue(); \
		\
		return _storage; \
	} \
	type operator* (const type& other) const \
	{ \
		return _storage * other; \
	}

#define __ObservableValueBinaryArithmeticDivision(type) \
	type& operator/= (const type& other) \
	{ \
		WillChangeValue(); \
		_storage /= other; \
		DidChangeValue(); \
		\
		return _storage; \
	} \
	type operator/ (const type& other) const \
	{ \
		return _storage / other; \
	}
	
#define __ObservableValueBinaryArithmetic(type) \
	__ObservableValueBinaryArithmeticAddition(type) \
	__ObservableValueBinaryArithmeticSubtraction(type) \
	__ObservableValueBinaryArithmeticMultiplication(type) \
	__ObservableValueBinaryArithmeticDivision(type)
	

#define __ObservableValueEnd() \
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
	
	__ObservableValueBegin(Vector2)
	__ObservableValueComparison(Vector2)
	__ObservableValueBinaryArithmetic(Vector2)
	__ObservableValuePointerLikes(Vector2)
	__ObservableValueEnd()

	__ObservableValueBegin(Vector3)
	__ObservableValueComparison(Vector3)
	__ObservableValueBinaryArithmetic(Vector3)
	__ObservableValuePointerLikes(Vector3)
	__ObservableValueEnd()
	
	__ObservableValueBegin(Vector4)
	__ObservableValueComparison(Vector4)
	__ObservableValueBinaryArithmetic(Vector4)
	__ObservableValuePointerLikes(Vector4)
	__ObservableValueEnd()
	
	__ObservableValueBegin(Color)
	__ObservableValueComparison(Color)
	__ObservableValueBinaryArithmetic(Color)
	__ObservableValuePointerLikes(Color)
	__ObservableValueEnd()
	
	__ObservableValueBegin(Matrix)
	__ObservableValueBinaryArithmeticMultiplication(Matrix)
	__ObservableValuePointerLikes(Matrix)
	__ObservableValueEnd()
	
	__ObservableValueBegin(Quaternion)
	__ObservableValueBinaryArithmetic(Quaternion)
	__ObservableValuePointerLikes(Quaternion)
	__ObservableValueEnd()
}

#endif /* __RAYNE_KVOIMPLEMENTATION */
