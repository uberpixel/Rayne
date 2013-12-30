//
//  RNKVOImplementation.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
	class ObservableScalar;
	
	template<class T>
	class ObservableValue;
	
	template<class T, class specialize = void>
	class Observable
	{
		Observable() = delete;
	};

#define __ObservableScalar(type, kvotype) \
	template<> \
	class ObservableScalar<type> : public ObservableProperty \
	{ \
	public: \
		typedef std::function<void (type)> SetterCallback; \
		typedef std::function<type (void)> GetterCallback; \
		ObservableScalar(const char *name, GetterCallback getter, SetterCallback setter) : \
			ObservableProperty(name, ObservableType::kvotype), \
			_getter(getter), \
			_setter(setter) \
		{} \
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
		Observable(const char *name, GetterCallback getter = GetterCallback(), SetterCallback setter = SetterCallback()) : \
			ObservableScalar(name, getter ? getter : std::bind(&Observable<type>::__BasicGetter, this), setter ? setter : std::bind(&Observable<type>::__BasicSetter, this, std::placeholders::_1)) \
		{} \
		Observable(const char *name, const type& initial, GetterCallback getter = GetterCallback(), SetterCallback setter = SetterCallback()) : \
			Observable(name, getter, setter) \
		{ \
			_storage = initial; \
		} \
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
		void __BasicSetter(const type& t) \
		{ \
			_storage = t; \
		} \
		const type& __BasicGetter() \
		{ \
			return _storage; \
		} \
		type _storage; \
	};

	
#define __ObservableValueBegin(type) \
	template<> \
	class ObservableValue<type> : public ObservableProperty \
	{ \
	public: \
		typedef std::function<void (const type&)> SetterCallback; \
		typedef std::function<type (void)> GetterCallback; \
		ObservableValue(const char *name, GetterCallback getter, SetterCallback setter) : \
			ObservableProperty(name, ObservableType::type), \
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
		Observable(const char *name, GetterCallback getter = GetterCallback(), SetterCallback setter = SetterCallback()) : \
			ObservableValue(name, getter ? getter : std::bind(&Observable<type>::__BasicGetter, this), setter ? setter : std::bind(&Observable<type>::__BasicSetter, this, std::placeholders::_1)) \
		{} \
		Observable(const char *name, const type& initial, GetterCallback getter, SetterCallback setter = SetterCallback()) : \
			Observable(name, getter, setter) \
		{ \
			_storage = initial; \
		} \
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
		void __BasicSetter(const type& other) \
		{ \
			_storage = other; \
		} \
		type& __BasicGetter() \
		{ \
			return _storage; \
		} \
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
	
	template<class T>
	class Observable<T *, typename std::enable_if<std::is_base_of<Object, T>::value>::type> : public ObservableProperty
	{
	public:
		typedef std::function<void (T *)> SetterCallback;
		typedef std::function<T *(void)> GetterCallback;
		
		Observable(const char *name, Object::MemoryPolicy policy, GetterCallback getter = GetterCallback(), SetterCallback setter = SetterCallback()) :
			ObservableProperty(name, ObservableType::Object),
			_policy(policy),
			_getter(getter),
			_setter(setter),
			_storage(nullptr)
		{}
		
		void SetValue(Object *object) override
		{
			WillChangeValue();
			
			if(!_setter)
			{
				switch(_policy)
				{
					case Object::MemoryPolicy::Assign:
						_storage = static_cast<T *>(object);
						break;
						
					case Object::MemoryPolicy::Retain:
						SafeRelease(_storage);
						_storage = static_cast<T *>(SafeRetain(object));
						break;
						
					case Object::MemoryPolicy::Copy:
						SafeRelease(_storage);
						_storage = object ? static_cast<T *>(object->Copy()) : nullptr;
						break;
				}
			}
			else
			{
				_setter(static_cast<T *>(object));
			}
		
			DidChangeValue();
		}
	
		Object *GetValue() const override
		{
			if(_getter)
				return _getter();
				
			return _storage;
		}

		bool operator== (T *other)
		{
			return (_storage->IsEqual(other));
		}

		bool operator!= (T *other)
		{
			return !(_storage->IsEqual(other));
		}

		operator bool()
		{
			return (_storage != nullptr);
		}

		operator T* ()
		{
			return _storage;
		}
		operator const T* () const
		{
			return _storage;
		}

		T *operator= (T *other)
		{
			WillChangeValue();
			_storage = other;
			DidChangeValue();
			
			return _storage;
		}

		T *operator ->()
		{
			return _storage;
		}
		const T *operator ->() const
		{
			return _storage;
		}

	private:
		SetterCallback _setter;
		GetterCallback _getter;
		Object::MemoryPolicy _policy;
		T *_storage;
	};
	
	template<>
	class ObservableValue<bool> : public ObservableProperty
	{
	public:
		typedef std::function<void (const bool&)> SetterCallback;
		typedef std::function<bool (void)> GetterCallback;
		
		ObservableValue(const char *name, GetterCallback getter, SetterCallback setter) :
			ObservableProperty(name, ObservableType::Bool),
			_getter(getter),
			_setter(setter)
		{}
		
		void SetValue(Object *value) override
		{
			RN_ASSERT(value->IsKindOfClass(Number::MetaClass()), "");
			Number *number = static_cast<Number *>(value);
		
			WillChangeValue();
			_setter(number->GetBoolValue());
			DidChangeValue();
		}
		
		Object *GetValue() const override
		{
			return Number::WithBool(_getter());
		}
	protected:
		SetterCallback _setter;
		GetterCallback _getter;
	};

	template<>
	class Observable<bool> : public ObservableValue<bool>
	{
	public:
		Observable(const char *name, GetterCallback getter = GetterCallback(), SetterCallback setter = SetterCallback()) :
			ObservableValue(name, getter ? getter : std::bind(&Observable<bool>::__BasicGetter, this), setter ? setter : std::bind(&Observable<bool>::__BasicSetter, this, std::placeholders::_1))
		{}
		Observable(const char *name, const bool& initial, GetterCallback getter = GetterCallback(), SetterCallback setter = SetterCallback()) :
			Observable(name, getter, setter)
		{
			_storage = initial;
		}
		
		bool operator== (const bool other)
		{
			return (_storage == other);
		}
		
		bool operator!= (const bool other)
		{
			return (_storage != other);
		}
		
		bool& operator= (const bool& other)
		{
			WillChangeValue();
			_storage = other;
			DidChangeValue();
		
			return _storage;
		}
		
		operator bool()
		{
			return _storage;
		}
		
		operator bool() const
		{
			return _storage;
		}
	private:
		void __BasicSetter(bool other)
		{
			_storage = other;
		}
		
		bool __BasicGetter()
		{
			return _storage;
		}
		
		bool _storage;
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
