//
//  RNObject.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OBJECT_H__
#define __RAYNE_OBJECT_H__

#include "RNBase.h"
#include "RNCatalogue.h"
#include "RNSpinLock.h"
#include "RNSignal.h"
#include "RNKVO.h"

namespace RN
{
	class Serializer;
	class Object
	{
	public:
		RNAPI Object();
		RNAPI virtual ~Object();
		
		RNAPI Object *Retain();
		RNAPI Object *Release();
		RNAPI Object *Autorelease();
		
		RNAPI Object *Copy() const;
		
		RNAPI virtual bool IsEqual(Object *other) const;
		RNAPI virtual machine_hash GetHash() const;
		RNAPI bool IsKindOfClass(MetaClassBase *other) const;
		RNAPI bool IsMemberOfClass(MetaClassBase *other) const;
		
		RNAPI virtual void Serialize(Serializer *serializer);
		
		RNAPI void Lock();
		RNAPI void Unlock();
		
		// -----------------
		// Type system
		// -----------------
		
		template<class T>
		T *Downcast()
		{
			static_assert(std::is_base_of<Object, T>::value, "T must inherit from Object!");
			
			if(IsKindOfClass(T::MetaClass()))
				return static_cast<T *>(this);
			
			throw Exception(Exception::Type::DowncastException, "No possible cast possible!");
		}
		
		virtual class MetaClassBase *Class() const
		{
			return Object::__metaClass;
		}
		static class MetaClassBase *MetaClass()
		{
			if(!__metaClass)
				__metaClass = new MetaType();
			
			return __metaClass;
		}
		
		static void InitialWakeUp(MetaClassBase *meta);
		
		// -----------------
		// Associated Objects
		// -----------------
		
		enum class MemoryPolicy
		{
			Assign,
			Retain,
			Copy
		};
		
		void SetAssociatedObject(const void *key, Object *value, MemoryPolicy policy);
		void RemoveAssociatedOject(const void *key);
		Object *GetAssociatedObject(const void *key);
		
		// -----------------
		// KVO / KVC
		// -----------------
		
		template<class F>
		void AddObserver(const std::string& keyPath, F&& function, void *cookie)
		{
			std::string key;
			ObservableProperty *property = GetPropertyForKeyPath(keyPath, key);
			
			if(!property)
				throw Exception(Exception::Type::InvalidArgumentException, "No property for key" + key);
			
			Connection *connection = property->_signal.Connect(std::move(function));
			MapCookie(cookie, connection);
		}
		
		void RemoveObserver(const std::string& keyPath, void *cookie)
		{
			std::string key;
			Object *object = ResolveKeyPath(keyPath, key);
			
			object->UnmapCookie(cookie);
		}
		
		void SetValueForKey(const std::string& keyPath, Object *value);
		Object *GetValueForKey(const std::string& keyPath);
		
		
	protected:
		virtual void CleanUp();
		
		void AddObservable(ObservableProperty *property);
		
		virtual void SetValueForUndefinedKey(const std::string& key, Object *value);
		virtual Object *GetValueForUndefinedKey(const std::string& key);
		
		void WillChangeValueForkey(const std::string& key);
		void DidChangeValueForKey(const std::string& key);
		
	private:
		class MetaType : public ConcreteMetaClass<Object>
		{
		public:
			MetaType() :
				MetaClassBase(0, "Object", __PRETTY_FUNCTION__)
			{}
		};
		
		void __RemoveAssociatedOject(const void *key);
		
		Object *ResolveKeyPath(const std::string& path, std::string& key);
		Object *GetPrimitiveValueForKey(const std::string& key);
		ObservableProperty *GetPropertyForKeyPath(const std::string& keyPath, std::string& key);
		
		void MapCookie(void *cookie, Connection *connection);
		void UnmapCookie(void *cookie);
		
		static MetaType *__metaClass;
		RecursiveSpinLock _lock;
		
		std::atomic<size_t> _refCount;
		std::atomic_flag _cleanUpFlag;
		std::unordered_map<void *, std::tuple<Object *, MemoryPolicy>> _associatedObjects;
		
		std::unordered_map<std::string, ObservableProperty *> _properties;
		std::unordered_map<void *, std::vector<Connection *>> _cookieMap;
	};
	
#define __RNDefineMetaPrivate(cls, super) \
	private: \
		class MetaType : public RN::ConcreteMetaClass<cls> \
		{ \
		public: \
			MetaType() : \
				MetaClassBase(super::MetaClass(), #cls, __PRETTY_FUNCTION__) \
			{} \
		}; \
		static MetaType *__##cls##__metaClass; \

#define __RNDefineMetaPrivateWithTraits(cls, super, ...) \
	private: \
		class MetaType : public RN::ConcreteMetaClass<cls, __VA_ARGS__> \
		{ \
		public: \
			MetaType() : \
				MetaClassBase(super::MetaClass(), #cls, __PRETTY_FUNCTION__) \
			{} \
		}; \
		static MetaType *__##cls##__metaClass; \

#define __RNDefineMetaPublic(cls) \
	public: \
		cls *Retain() \
		{ \
			return static_cast<cls *>(Object::Retain()); \
		} \
		cls *Release() \
		{ \
			return static_cast<cls *>(Object::Release()); \
		} \
		cls *Autorelease() \
		{ \
			return static_cast<cls *>(Object::Autorelease()); \
		} \
		cls *Copy() const \
		{ \
			return static_cast<cls *>(Object::Copy()); \
		} \
		class RN::MetaClassBase *Class() const override \
		{ \
			return cls::__##cls##__metaClass; \
		} \
		static class RN::MetaClassBase *MetaClass() \
		{ \
			if(!cls::__##cls##__metaClass) \
				cls::__##cls##__metaClass = new cls::MetaType(); \
			return cls::__##cls##__metaClass; \
		}
	
#define RNDefineMeta(cls, super) \
	__RNDefineMetaPrivate(cls, super) \
	__RNDefineMetaPublic(cls)
	
#define RNDefineMetaWithTraits(cls, super, ...) \
	__RNDefineMetaPrivateWithTraits(cls, super, __VA_ARGS__) \
	__RNDefineMetaPublic(cls)

#define RNDeclareMeta(cls) \
	cls::MetaType *cls::__##cls##__metaClass = nullptr; \
	RN_REGISTER_INIT(cls##Init, cls::MetaClass(); cls::InitialWakeUp(cls::MetaClass()))

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
			return object1->IsEqual(const_cast<RN::Object *>(object2));
		}
	};
}

#endif /* __RAYNE_OBJECT_H__ */
