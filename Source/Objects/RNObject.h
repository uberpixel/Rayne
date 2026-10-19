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
#include "../Threads/RNLockable.h"
#include "RNCatalogue.h"
#include "RNKVO.h"

#include <source_location>

#define RN_ZOMBIE_ALLOCATION 0

namespace RN
{
	class Serializer;
	class String;

	class Object
	{
	public:
		friend class AutoreleasePool;

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
		RNAPI Object *Retain_debug(const char* file, int line, const char* func);
		RNAPI const Object *Retain_debug(const char* file, int line, const char* func) const;
		RNAPI void Release_debug(const char* file, int line, const char* func) const;
		RNAPI Object *Autorelease_debug(const char* file, int line, const char* func);
		RNAPI const Object *Autorelease_debug(const char* file, int line, const char* func) const;
		
		RN_INLINE Object *Retain(const std::source_location &loc = std::source_location::current())
		{
			return Retain_debug(loc.file_name(), loc.line(), loc.function_name());
		}
		RN_INLINE const Object *Retain(const std::source_location &loc = std::source_location::current()) const
		{
			return Retain_debug(loc.file_name(), loc.line(), loc.function_name());
		}
		RN_INLINE void Release(const std::source_location &loc = std::source_location::current()) const
		{
			Release_debug(loc.file_name(), loc.line(), loc.function_name());
		}
		RN_INLINE Object *Autorelease(const std::source_location &loc = std::source_location::current())
		{
			return Autorelease_debug(loc.file_name(), loc.line(), loc.function_name());
		}
		RN_INLINE const Object *Autorelease(const std::source_location &loc = std::source_location::current()) const
		{
			return Autorelease_debug(loc.file_name(), loc.line(), loc.function_name());
		}
#else
		RNAPI Object *Retain();
		RNAPI const Object *Retain() const;
		RNAPI void Release() const;
		RNAPI Object *Autorelease();
		RNAPI const Object *Autorelease() const;
#endif

		RNAPI virtual const String *GetDescription() const;

		RNAPI Object *Copy() const;

		RNAPI virtual bool IsEqual(const Object *other) const;
		RNAPI virtual size_t GetHash() const;
		RNAPI bool IsKindOfClass(const MetaClass *other) const;

		RNAPI virtual void Serialize(Serializer *serializer) const;

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
		RNAPI void StartReferenceTracking();
		RNAPI void StopReferenceTracking();

		RNAPI virtual void WillChangeReferenceCount(size_t refCount) const;
		RNAPI virtual void WillChangeAutoreleaseCount(size_t autoreleaseCount) const;
#endif

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
		void AddObserver(const char *key, F &&function, void *cookie) const
		{
			ObservableProperty *property = GetPropertyForKey(key);

			if(!property)
				throw InvalidArgumentException("No property for key ");

			LockGuard<RecursiveLockable> lock(const_cast<RecursiveLockable &>(_lock));
			property->AssertSignal();

			Connection *connection = property->_signal->Connect(std::move(function));
			MapCookie(cookie, property, connection);
		}

		void RemoveObserver(const char *key, void *cookie) const
		{
			ObservableProperty *property = GetPropertyForKey(key);
			UnmapCookie(cookie, property);
		}

		RNAPI void SetValueForKey(Object *value, const char *key);
		RNAPI void SetValueForKeyPath(Object *value, const char *keyPath);

		template<class T = Object>
		T *GetValueForKey(const char *key) const
		{
			Object *result = GetPrimitiveValueForKey(key);
			return result ? result->Downcast<T>() : nullptr;
		}

		template<class T = Object>
		T *GetValueForKeyPath(const char *keyPath) const
		{
			Object *result = GetPrimitiveValueForKeyPath(keyPath);
			return result ? result->Downcast<T>() : nullptr;
		}


		RNAPI std::vector<ObservableProperty *> GetPropertiesForClass(MetaClass *meta);

	protected:
		RNAPI Object();
		RNAPI virtual ~Object();
		RNAPI virtual void Dealloc();

		RNAPI void AddObservable(ObservableProperty *property);
		RNAPI void AddObservables(const std::initializer_list<ObservableProperty *> properties);

		RNAPI virtual void SetValueForUndefinedKey(Object *value, const char *key);
		RNAPI virtual Object *GetValueForUndefinedKey(const char *key) const;

		RNAPI void WillChangeValueForKey(const char *key);
		RNAPI void DidChangeValueForKey(const char *key);

	private:
		class MetaType : public __ConcreteMetaClass<Object>
		{
		public:
			MetaType() :
				MetaClass(0, "Object", RN_FUNCTION_SIGNATURE)
			{}
		};

		void __RemoveAssociatedObject(const void *key);

		RNAPI void SetPrimitiveValueForKey(Object *value, const char *key);
		RNAPI Object *GetPrimitiveValueForKey(const char *key) const;
		RNAPI Object *GetPrimitiveValueForKeyPath(const char *keyPath) const;

		RNAPI ObservableProperty *GetPropertyForKey(const char *key) const;

		RNAPI void MapCookie(void *cookie, ObservableProperty *property, Connection *connection) const;
		RNAPI void UnmapCookie(void *cookie, ObservableProperty *property) const;

		RecursiveLockable _lock;

#if RN_ZOMBIE_ALLOCATION
		mutable bool _isZombie;
#endif
		mutable std::atomic<size_t> _refCount;

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
		// Strong references not yet consumed by Release() or reserved by an autorelease pool.
		// One atomic balance avoids comparing reference and pool counters sampled at different times.
		mutable std::atomic<size_t> _availableReferenceCount;
		mutable std::atomic<size_t> _autoreleaseCounter;
		mutable bool _isTracked;
#endif

		std::unordered_map<void *, std::tuple<Object *, MemoryPolicy>> _associatedObjects;

		std::vector<ObservableProperty *> _properties;
		mutable std::vector<std::tuple<void *, ObservableProperty *, Connection *>> _cookies;
	};

#define __RNDeclareMetaPrivateWithTraits(cls, super, ...)\
	class cls##MetaType : public RN::__ConcreteMetaClass<cls, __VA_ARGS__>\
	{\
	public:\
		cls##MetaType(const char *signature) :\
			MetaClass(super::GetMetaClass(), #cls, signature)\
		{}\
	};

#define __RNDeclareScopedMetaPrivateWithTraits(scope, cls, super, ...)\
	class scope##cls##MetaType : public RN::__ConcreteMetaClass<scope::cls, __VA_ARGS__>\
	{\
	public:\
		scope##cls##MetaType(const char *signature) :\
			MetaClass(super::GetMetaClass(), #cls, signature)\
		{}\
	};

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
#define __RNDeclareMetaPublic(cls)\
public:\
	RN_INLINE cls *Retain(const std::source_location &loc = std::source_location::current())\
	{\
		return static_cast<cls *>(Object::Retain(loc));\
	}\
	RN_INLINE const cls *Retain(const std::source_location &loc = std::source_location::current()) const\
	{\
		return static_cast<const cls *>(Object::Retain(loc));\
	}\
	RN_INLINE cls *Autorelease(const std::source_location &loc = std::source_location::current())\
	{\
		return static_cast<cls *>(Object::Autorelease(loc));\
	}\
	RN_INLINE const cls *Autorelease(const std::source_location &loc = std::source_location::current()) const\
	{\
		return static_cast<const cls *>(Object::Autorelease(loc));\
	}\
	RN_INLINE cls *Copy() const\
	{\
		return static_cast<cls *>(Object::Copy());\
	}
#else
#define __RNDeclareMetaPublic(cls)\
public:\
	cls *Retain()\
	{\
		return static_cast<cls *>(Object::Retain());\
	}\
	const cls *Retain() const\
	{\
		return static_cast<const cls *>(Object::Retain());\
	}\
	cls *Autorelease()\
	{\
		return static_cast<cls *>(Object::Autorelease());\
	}\
	const cls *Autorelease() const\
	{\
		return static_cast<const cls *>(Object::Autorelease());\
	}\
	cls *Copy() const\
	{\
		return static_cast<cls *>(Object::Copy());\
	}
#endif

#define __RNDeclareMetaInternal(cls)\
	__RNDeclareMetaPublic(cls)\
	RNAPI MetaClass *GetClass() const override;\
	RNAPI static MetaClass *GetMetaClass();

#define RNDeclareMeta(cls)\
	__RNDeclareMetaPublic(cls)\
	RN::MetaClass *GetClass() const override;\
	static RN::MetaClass *GetMetaClass();

#define RNDeclareMetaAPI(cls, apiVisibility)\
	__RNDeclareMetaPublic(cls)\
	apiVisibility RN::MetaClass *GetClass() const override;\
	apiVisibility static RN::MetaClass *GetMetaClass();

#if RN_BUILD_LIBRARY
#define __RNObjectInitializer(cls, name)\
	RN_REGISTER_INITIALIZER(name##Init, cls::InitialWakeUp(cls::GetMetaClass()))
#else
#define __RNObjectInitializer(cls, name)\
	namespace\
	{\
		static void *__RN##name##Register()\
		{\
			cls::InitialWakeUp(cls::GetMetaClass());\
			return reinterpret_cast<void *>(&cls::GetMetaClass);\
		}\
		RN_REGISTER_INITIALIZER(name##init, RN::__RegisterMetaClass(&__RN##name##Register))\
	}
#endif

#define RNDefineMeta(cls, super)\
	static_assert(std::is_base_of<super, cls>::value, "RNDefineMeta() with an invalid object inheritance");\
	__RNDeclareMetaPrivateWithTraits(\
		cls, super,\
		std::conditional<std::is_default_constructible<cls>::value && !std::is_abstract<cls>::value, RN::MetaClassTraitConstructable<cls>, RN::__MetaClassTraitNull0<cls>>::type,\
		std::conditional<std::is_constructible<cls, RN::Deserializer *>::value && !std::is_abstract<cls>::value, RN::MetaClassTraitSerializable<cls>, RN::__MetaClassTraitNull1<cls>>::type,\
		std::conditional<std::is_constructible<cls, const cls *>::value && !std::is_abstract<cls>::value, RN::MetaClassTraitCopyable<cls>, RN::__MetaClassTraitNull2<cls>>::type)\
	RN::MetaClass *cls::GetClass() const\
	{\
		return cls::GetMetaClass();\
	}\
	RN::MetaClass *cls::GetMetaClass()\
	{\
		static cls##MetaType *meta = new cls##MetaType(RN_FUNCTION_SIGNATURE);\
		return meta;\
	}\
	__RNObjectInitializer(cls, cls)

#define RNDefineScopedMeta(scope, cls, super)\
	__RNDeclareScopedMetaPrivateWithTraits(\
		scope, cls, super,\
		std::conditional<std::is_default_constructible<scope::cls>::value && !std::is_abstract<scope::cls>::value, RN::MetaClassTraitConstructable<scope::cls>, RN::__MetaClassTraitNull0<scope::cls>>::type,\
		std::conditional<std::is_constructible<scope::cls, RN::Deserializer *>::value && !std::is_abstract<scope::cls>::value, RN::MetaClassTraitSerializable<scope::cls>, RN::__MetaClassTraitNull1<scope::cls>>::type,\
		std::conditional<std::is_constructible<scope::cls, const scope::cls *>::value && !std::is_abstract<scope::cls>::value, RN::MetaClassTraitCopyable<scope::cls>, RN::__MetaClassTraitNull2<scope::cls>>::type)\
	RN::MetaClass *scope::cls::GetClass() const\
	{\
		return scope::cls::GetMetaClass();\
	}\
	RN::MetaClass *scope::cls::GetMetaClass()\
	{\
		static scope##cls##MetaType *meta = new scope##cls##MetaType(RN_FUNCTION_SIGNATURE);\
		return meta;\
	}\
	__RNObjectInitializer(scope::cls, cls)

#define __RNDefineMetaAndGFYMSVC(cls, super)\
	__RNDeclareMetaPrivateWithTraits(cls, super, RN::__MetaClassTraitNull0<cls>, RN::__MetaClassTraitNull1<cls>, RN::__MetaClassTraitNull2<cls>)\
	RN::MetaClass *cls::GetClass() const\
	{\
		return cls::GetMetaClass();\
	}\
	RN::MetaClass *cls::GetMetaClass()\
	{\
		static cls##MetaType *meta = new cls##MetaType(RN_FUNCTION_SIGNATURE);\
		return meta;\
	}\
	RN_REGISTER_INITIALIZER(cls##Init, cls::GetMetaClass(); cls::InitialWakeUp(cls::GetMetaClass()))

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
	template<class T>
	static void SafeRelease(T *&object, const std::source_location &loc = std::source_location::current())
	{
		if(object)
		{
			object->Release(loc);
			object = nullptr;
		}
	}

	template<class T>
	static void SafeRelease(T &object, const std::source_location &loc = std::source_location::current())
	{
		static_assert(std::is_base_of<ObservableProperty, T>::value, "T must be of type ObservableProperty");

		if(object)
		{
			object->Release(loc);
			object = nullptr;
		}
	}

	template<class T>
	static T *SafeRetain(T *object, const std::source_location &loc = std::source_location::current())
	{
		return object ? static_cast<T *>(object->Retain(loc)) : nullptr;
	}

	template<class T>
	static const T *SafeRetain(const T *object, const std::source_location &loc = std::source_location::current())
	{
		return object ? static_cast<const T *>(object->Retain(loc)) : nullptr;
	}
#else
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
	static const T *SafeRetain(const T *object)
	{
		return object ? static_cast<const T *>(object->Retain()) : nullptr;
	}
#endif

	template<class T>
	static T *SafeCopy(const T *object)
	{
		return object ? static_cast<T *>(object->Copy()) : nullptr;
	}

	template<class T>
	static const T *SafeDowncast(const Object *object)
	{
		return object ? object->Downcast<T>() : nullptr;
	}

	template<class T>
	static T *SafeDowncast(Object *object)
	{
		return object ? object->Downcast<T>() : nullptr;
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

		StrongRef(StrongRef<T> &&other) noexcept :
			_value(std::exchange(other._value, nullptr))
		{
		}

		~StrongRef()
		{
			if(_value)
				_value->Release();
		}

		StrongRef &operator=(const StrongRef<T> &other)
		{
			Assign(other._value);
			return *this;
		}

		StrongRef& operator=(StrongRef<T> &&other) noexcept
		{
			StrongRef<T> tmp(std::move(other));
			swap(*this, tmp);
			return *this;
		}

		StrongRef &operator=(T *value)
		{
			Assign(value);
			return *this;
		}

		operator T *() const
		{
			return Load();
		}

		T *Get() const
		{
			return _value;
		}


		T *operator->() const
		{
			return _value;
		}

		T *Load() const
		{
			T *object = _value;
			if(!object) return object;

			object->Retain();
			object->Autorelease();

			return object;
		}

		friend void swap(StrongRef<T> &a, StrongRef<T> &b) noexcept
		{
			using std::swap;
			swap(a._value, b._value);
		}

	private:
		void Assign(T *value)
		{
			T *temp = _value;

			if((_value = value))
				_value->Retain();
			
			if(temp)
				temp->Release();
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


		WeakRef &operator=(const WeakRef<T> &other)
		{
			__StoreWeak(reinterpret_cast<Object **>(&_reference), static_cast<Object *>(other.Load()));
			return *this;
		}
		WeakRef &operator=(const StrongRef<T> &other)
		{
			__StoreWeak(reinterpret_cast<Object **>(&_reference), static_cast<Object *>(other.Load()));
			return *this;
		}
		WeakRef &operator=(T *value)
		{
			__StoreWeak(reinterpret_cast<Object **>(&_reference), static_cast<Object *>(value));
			return *this;
		}


		operator T *() const
		{
			return Load();
		}

		T *operator->() const
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

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
	class RefcountDebugGraph
	{
	public:
		struct RefcountCallEvent
		{
			enum Type { Retain, Release, Autorelease } type;
			const char* file;
			int line;
			const char* func;
			int afterRefCount;
			std::string stackTrace;
		};

		struct RefcountDebugInfo
		{
			std::string className;
			std::string object;
			std::vector<RefcountCallEvent> events;
		};

		static RefcountDebugGraph& GetSharedInstance()
		{
			static RefcountDebugGraph inst;
			return inst;
		}

		void AddRefcountDebugInfo(const Object* obj, RefcountCallEvent::Type type, const char* file, int line, const char* func, int afterRefCount)
		{
			std::lock_guard<std::mutex> lk(_mu);

			auto [it, inserted] = _refcountDebugMap.try_emplace(obj, RefcountDebugInfo{});

			if(inserted)
			{
				it->second.className = obj->GetClass()->GetFullname();
				it->second.object = std::to_string(reinterpret_cast<uintptr_t>(obj));
			}

			RefcountCallEvent event{type, file, line, func, afterRefCount};
			event.stackTrace = CaptureStackTrace();
			it->second.events.push_back(std::move(event));
		}

		RNAPI void DumpAll(const std::string &filePath, bool onlyStillLive = false);

	private:
		RefcountDebugGraph() = default;
		std::string CaptureStackTrace();

		std::mutex _mu;
		std::unordered_map<const Object*, RefcountDebugInfo> _refcountDebugMap;
	};
#endif

#define RNObjectClass(name)\
	using name##Ref = RN::StrongRef<name>;\
	using Weak##name = RN::WeakRef<name>;

#define RNObjectTransferRef(t)\
	const_cast<decltype(t)>((t)->Autorelease())
} // namespace RN

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
} // namespace std

#endif /* __RAYNE_OBJECT_H__ */
