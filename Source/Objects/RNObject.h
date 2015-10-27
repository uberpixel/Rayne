//
//  RNObject.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OBJECT_H__
#define __RAYNE_OBJECT_H__

#include "../Base/RNBase.h"
#include "../Base/RNSignal.h"
#include "../Threads/RNSpinLock.h"
#include "RNCatalogue.h"
#include "RNKVO.h"

namespace RN
{
	class Serializer;
	class String;

	class Object
	{
	public:
		RNAPI Object *Retain();
		RNAPI void Release();
		RNAPI Object *Autorelease();

		RNAPI virtual const String *GetDescription() const;
		
		RNAPI Object *Copy() const;
		
		RNAPI virtual bool IsEqual(const Object *other) const;
		RNAPI virtual size_t GetHash() const;
		RNAPI bool IsKindOfClass(const MetaClass *other) const;
		
		RNAPI virtual void Serialize(Serializer *serializer) const;
		
		RNAPI void Lock();
		RNAPI void Unlock();
		
		// -----------------
		// Type system
		// -----------------
		
		template<class T>
		T *Downcast()
		{
			static_assert(std::is_base_of<Object, T>::value, "T must inherit from Object!");
			
			if(IsKindOfClass(T::GetMetaClass()))
				return static_cast<T *>(this);
			
			return nullptr;
		}

		template<class T>
		const T *Downcast() const
		{
			static_assert(std::is_base_of<Object, T>::value, "T must inherit from Object!");

			if(IsKindOfClass(T::GetMetaClass()))
				return static_cast<const T *>(this);

			return nullptr;
		}
		
		RNAPI virtual MetaClass *GetClass() const;
		RNAPI static MetaClass *GetMetaClass();
		
		RNAPI static void InitialWakeUp(MetaClass *meta);
		
		// -----------------
		// Associated Objects
		// -----------------
		
		enum class MemoryPolicy
		{
			Assign,
			Retain,
			Copy
		};
		
		RNAPI void SetAssociatedObject(const void *key, Object *value, MemoryPolicy policy);
		RNAPI Object *GetAssociatedObject(const void *key);
		
		// -----------------
		// KVO / KVC
		// -----------------
		
		template<class F>
		void AddObserver(const std::string &keyPath, F &&function, void *cookie)
		{
			std::string key;
			ObservableProperty *property = GetPropertyForKeyPath(keyPath, key);
			
			if(!property)
				throw InvalidArgumentException("No property for key");

			Lock();
			property->AssertSignal();
			
			Connection *connection = property->_signal->Connect(std::move(function));
			MapCookie(cookie, property, connection);
			
			Unlock();
		}
		
		void RemoveObserver(const std::string &keyPath, void *cookie)
		{
			std::string key;
			
			ObservableProperty *property = GetPropertyForKeyPath(keyPath, key);
			Object *object = property->_object;
			
			object->UnmapCookie(cookie, property);
		}
		
		RNAPI void SetValueForKey(Object *value, const std::string &keyPath);
		RNAPI Object *GetValueForKey(const std::string &keyPath);
		RNAPI std::vector<ObservableProperty *> GetPropertiesForClass(MetaClass *meta);
		
	protected:
		RNAPI Object();
		RNAPI virtual ~Object();
		RNAPI virtual void Dealloc();
		
		RNAPI void AddObservable(ObservableProperty *property);
		RNAPI void AddObservables(std::initializer_list<ObservableProperty *> properties);
		
		RNAPI virtual void SetValueForUndefinedKey(Object *value, const std::string &key);
		RNAPI virtual Object *GetValueForUndefinedKey(const std::string &key);
		
		RNAPI void WillChangeValueForKey(const std::string &key);
		RNAPI void DidChangeValueForKey(const std::string &key);
		
	private:
		class MetaType : public __ConcreteMetaClass<Object>
		{
		public:
			MetaType() :
				MetaClass(0, "Object", RN_FUNCTION_SIGNATURE)
			{}
		};

		void __RemoveAssociatedObject(const void *key);
		
		RNAPI Object *ResolveKeyPath(const std::string &path, std::string &key);
		RNAPI Object *GetPrimitiveValueForKey(const std::string &key);
		RNAPI ObservableProperty *GetPropertyForKeyPath(const std::string &keyPath, std::string &key);
		
		RNAPI void MapCookie(void *cookie, ObservableProperty *property, Connection *connection);
		RNAPI void UnmapCookie(void *cookie, ObservableProperty *property);
		
		RecursiveSpinLock _lock;
		
		std::atomic<size_t> _refCount;
		std::unordered_map<void *, std::tuple<Object *, MemoryPolicy>> _associatedObjects;
		
		std::vector<ObservableProperty *> _properties;
		std::vector<std::tuple<void *, ObservableProperty *, Connection *>> _cookies;
	};
	
#define __RNDeclareMetaPrivateWithTraits(cls, super, ...) \
		class cls##MetaType : public RN::__ConcreteMetaClass<cls, __VA_ARGS__> \
		{ \
		public: \
			cls##MetaType(const char *signature) : \
				MetaClass(super::GetMetaClass(), #cls, signature) \
			{} \
		};
#define __RNDeclareScopedMetaPrivateWithTraits(scope, cls, super, ...) \
		class scope##cls##MetaType : public RN::__ConcreteMetaClass<scope::cls, __VA_ARGS__> \
		{ \
		public: \
			scope##cls##MetaType(const char *signature) : \
				MetaClass(super::GetMetaClass(), #cls, signature) \
			{} \
		};

#define __RNDeclareMetaPublic(cls) \
	public: \
		cls *Retain() \
		{ \
			return static_cast<cls *>(Object::Retain()); \
		} \
		cls *Autorelease() \
		{ \
			return static_cast<cls *>(Object::Autorelease()); \
		} \
		cls *Copy() const \
		{ \
			return static_cast<cls *>(Object::Copy()); \
		} \
		RNAPI_DEFINEBASE RN::MetaClass *GetClass() const override; \
		RNAPI_DEFINEBASE static RN::MetaClass *GetMetaClass();
	
#define RNDeclareMeta(cls) \
	__RNDeclareMetaPublic(cls)

#define RNDefineMeta(cls, super) \
	__RNDeclareMetaPrivateWithTraits(cls, super, \
		std::conditional<std::is_default_constructible<cls>::value && !std::is_abstract<cls>::value, RN::MetaClassTraitCronstructable<cls>, RN::__MetaClassTraitNull0<cls>>::type, \
		std::conditional<std::is_constructible<cls, RN::Deserializer *>::value && !std::is_abstract<cls>::value, RN::MetaClassTraitSerializable<cls>, RN::__MetaClassTraitNull1<cls>>::type, \
		std::conditional<std::is_constructible<cls, const cls *>::value && !std::is_abstract<cls>::value, RN::MetaClassTraitCopyable<cls>, RN::__MetaClassTraitNull2<cls>>::type) \
	void *__kRN##cls##__metaClass = nullptr; \
	RN::MetaClass *cls::GetClass() const \
	{ \
		return cls::GetMetaClass(); \
	} \
	RN::MetaClass *cls::GetMetaClass() \
	{ \
		if(!__kRN##cls##__metaClass) \
			__kRN##cls##__metaClass = new cls##MetaType(RN_FUNCTION_SIGNATURE); \
		return reinterpret_cast<cls##MetaType *>(__kRN##cls##__metaClass); \
	} \
	RN_REGISTER_INITIALIZER(cls##Init, cls::GetMetaClass(); cls::InitialWakeUp(cls::GetMetaClass()))

#define RNDefineScopedMeta(scope, cls, super) \
	__RNDeclareScopedMetaPrivateWithTraits(scope, cls, super, \
		std::conditional<std::is_default_constructible<scope::cls>::value && !std::is_abstract<scope::cls>::value, RN::MetaClassTraitCronstructable<scope::cls>, RN::__MetaClassTraitNull0<scope::cls>>::type, \
		std::conditional<std::is_constructible<scope::cls, RN::Deserializer *>::value && !std::is_abstract<scope::cls>::value, RN::MetaClassTraitSerializable<scope::cls>, RN::__MetaClassTraitNull1<scope::cls>>::type, \
		std::conditional<std::is_constructible<scope::cls, const scope::cls *>::value && !std::is_abstract<scope::cls>::value, RN::MetaClassTraitCopyable<scope::cls>, RN::__MetaClassTraitNull2<scope::cls>>::type) \
	void *__kRN##scope##cls##__metaClass = nullptr; \
	RN::MetaClass *scope::cls::GetClass() const \
	{ \
		return scope::cls::GetMetaClass(); \
	} \
	RN::MetaClass *scope::cls::GetMetaClass() \
	{ \
		if(!__kRN##scope##cls##__metaClass) \
			__kRN##scope##cls##__metaClass = new scope##cls##MetaType(RN_FUNCTION_SIGNATURE); \
		return reinterpret_cast<scope##cls##MetaType *>(__kRN##scope##cls##__metaClass); \
	} \
	RN_REGISTER_INITIALIZER(cls##Init, scope::cls::GetMetaClass(); scope::cls::InitialWakeUp(scope::cls::GetMetaClass()))
	
#define __RNDefineMetaAndGFYMSVC(cls, super) \
	__RNDeclareMetaPrivateWithTraits(cls, super, RN::__MetaClassTraitNull0<cls>, RN::__MetaClassTraitNull1<cls>, RN::__MetaClassTraitNull2<cls>) \
	void *__kRN##cls##__metaClass = nullptr; \
	RN::MetaClass *cls::GetClass() const \
	{ \
		return cls::GetMetaClass(); \
	} \
	RN::MetaClass *cls::GetMetaClass() \
	{ \
		if(!__kRN##cls##__metaClass) \
			__kRN##cls##__metaClass = new cls##MetaType(RN_FUNCTION_SIGNATURE); \
		return reinterpret_cast<cls##MetaType *>(__kRN##cls##__metaClass); \
	} \
	RN_REGISTER_INITIALIZER(cls##Init, cls::GetMetaClass(); cls::InitialWakeUp(cls::GetMetaClass()))

	template<class T>
	static void SafeRelease(T *&object)
	{
		if(object)
		{
			object->Release();
			object = nullptr;
		}
	}
	
	template<class T>
	static void SafeRelease(T &object)
	{
		static_assert(std::is_base_of<ObservableProperty, T>::value, "T must be of type ObservableProperty");
		
		if(object)
		{
			object->Release();
			object = nullptr;
		}
	}
	
	template<class T>
	static T *SafeRetain(T *object)
	{
		return object ? static_cast<T *>(object->Retain()) : nullptr;
	}

	template<class T>
	static T *SafeCopy(const T *object)
	{
		return object ? static_cast<T *>(object->Copy()) : nullptr;
	}
	
	// ---------------------
	// MARK: -
	// MARK: Smart pointers
	// ---------------------
	
	template<class T>
	struct StrongRef
	{
		StrongRef() :
			_value(nullptr)
		{}
		
		StrongRef(T *value) :
			_value(nullptr)
		{
			Assign(value);
		}
		
		StrongRef(const StrongRef<T> &other) :
			_value(nullptr)
		{
			Assign(other._value);
		}
		~StrongRef()
		{
			if(_value)
				_value->Release();
		}
		
		StrongRef &operator =(const StrongRef<T> &other)
		{
			Assign(other._value);
			return *this;
		}
		
		StrongRef &operator =(T *value)
		{
			Assign(value);
			return *this;
		}
		
		operator T* () const
		{
			return Load();
		}
		
		
		T *operator ->() const
		{
			return _value;
		}
		
		T *Load() const
		{
			T *object = _value;
			
			object->Retain();
			object->Autorelease();
			
			return object;
		}
		
	private:
		void Assign(T *value)
		{
			if(_value)
				_value->Release();
			
			if((_value = value))
				_value->Retain();
		}
		
		T *_value;
	};
	
	RNAPI Object *__InitWeak(Object **weak, Object *value);
	RNAPI Object *__StoreWeak(Object **weak, Object *value);
	RNAPI Object *__LoadWeakObjectRetained(Object **weak);
	RNAPI Object *__RemoveWeakObject(Object **weak);
	
	template<class T>
	struct WeakRef
	{
		WeakRef()
		{
			__InitWeak(reinterpret_cast<Object **>(&_reference), nullptr);
		}
		WeakRef(T *value)
		{
			__InitWeak(reinterpret_cast<Object **>(&_reference), static_cast<Object *>(value));
		}
		WeakRef(const WeakRef<T> &other)
		{
			__InitWeak(reinterpret_cast<Object **>(&_reference), static_cast<Object *>(other.Load()));
		}
		WeakRef(const StrongRef<T> &other)
		{
			__InitWeak(reinterpret_cast<Object **>(&_reference), static_cast<Object *>(other.Load()));
		}
		
		~WeakRef()
		{
			__RemoveWeakObject(reinterpret_cast<Object **>(&_reference));
		}
		
		
		WeakRef &operator =(const WeakRef<T> &other)
		{
			__StoreWeak(reinterpret_cast<Object **>(&_reference), static_cast<Object *>(other.Load()));
			return *this;
		}
		WeakRef &operator =(const StrongRef<T> &other)
		{
			__StoreWeak(reinterpret_cast<Object **>(&_reference), static_cast<Object *>(other.Load()));
			return *this;
		}
		WeakRef &operator =(T *value)
		{
			__StoreWeak(reinterpret_cast<Object **>(&_reference), static_cast<Object *>(value));
			return *this;
		}
		
		
		operator T* () const
		{
			return Load();
		}
		
		T *operator ->() const
		{
			return const_cast<T *>(Load());
		}
		
		T *Load() const
		{
			Object *object = __LoadWeakObjectRetained(reinterpret_cast<Object **>(&_reference));
			if(!object)
				return nullptr;
			
			return static_cast<T *>(object->Autorelease());
		}
		
	private:
		mutable T *_reference;
	};
	
#define RNObjectClass(name) \
	using name##Ref = RN::StrongRef<name>; \
	using Weak##name = RN::WeakRef<name>;
	
#define RNObjectTransferRef(t) \
	const_cast<decltype(t)>((t)->Autorelease())
}

namespace std
{
	template<>
	struct hash<RN::Object>
	{
		size_t operator()(const RN::Object *object) const
		{
			return static_cast<size_t>(object->GetHash());
		}
	};
	
	template<>
	struct equal_to<RN::Object>
	{
		bool operator()(const RN::Object *object1, const RN::Object *object2) const
		{
			return object1->IsEqual(object2);
		}
	};
}

#endif /* __RAYNE_OBJECT_H__ */
