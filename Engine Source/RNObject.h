//
//  RNObject.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
		
		RNAPI virtual MetaClassBase *Class() const;
		RNAPI static MetaClassBase *MetaClass();
		
		RNAPI static void InitialWakeUp(MetaClassBase *meta);
		
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
		RNAPI void RemoveAssociatedOject(const void *key);
		RNAPI Object *GetAssociatedObject(const void *key);
		
		// -----------------
		// KVO / KVC
		// -----------------
		
		template<class F>
		void AddObserver(const std::string& keyPath, F&& function, void *cookie)
		{
			std::string key;
			ObservableProperty *property = GetPropertyForKeyPath(keyPath, key);
			
			if(!property)
				throw Exception(Exception::Type::InvalidArgumentException, "No property for key \"%s\"", key.c_str());
			
			Lock();
			property->AssertSignal();
			Unlock();
			
			Connection *connection = property->_signal->Connect(std::move(function));
			MapCookie(cookie, property, connection);
		}
		
		void RemoveObserver(const std::string& keyPath, void *cookie)
		{
			std::string key;
			
			ObservableProperty *property = GetPropertyForKeyPath(keyPath, key);
			Object *object = property->_object;
			
			object->UnmapCookie(cookie, property);
		}
		
		RNAPI void SetValueForKey(Object *value, const std::string& keyPath);
		RNAPI Object *GetValueForKey(const std::string& keyPath);
		RNAPI std::vector<ObservableProperty *> GetPropertiesForClass(MetaClassBase *meta);
		
	protected:
		RNAPI virtual void CleanUp();
		
		RNAPI void AddObservable(ObservableProperty *property);
		RNAPI void AddObservables(std::initializer_list<ObservableProperty *> properties);
		
		RNAPI virtual void SetValueForUndefinedKey(Object *value, const std::string& key);
		RNAPI virtual Object *GetValueForUndefinedKey(const std::string& key);
		
		RNAPI void WillChangeValueForkey(const std::string& key);
		RNAPI void DidChangeValueForKey(const std::string& key);
		
	private:
		class MetaType : public ConcreteMetaClass<Object>
		{
		public:
			MetaType() :
				MetaClassBase(0, "Object", RN_FUNCTION_SIGNATURE)
			{}
		};
		
		void __RemoveAssociatedOject(const void *key);
		
		Object *ResolveKeyPath(const std::string& path, std::string& key);
		Object *GetPrimitiveValueForKey(const std::string& key);
		ObservableProperty *GetPropertyForKeyPath(const std::string& keyPath, std::string& key);
		
		void MapCookie(void *cookie, ObservableProperty *property, Connection *connection);
		void UnmapCookie(void *cookie, ObservableProperty *property);
		
		RecursiveSpinLock _lock;
		
		std::atomic<size_t> _refCount;
		std::atomic_flag _cleanUpFlag;
		std::unordered_map<void *, std::tuple<Object *, MemoryPolicy>> _associatedObjects;
		
		std::vector<ObservableProperty *> _properties;
		std::vector<std::tuple<void *, ObservableProperty *, Connection *>> _cookies;
	};
	
#define __RNDeclareMetaPrivate(cls, super) \
	private: \
		class MetaType : public RN::ConcreteMetaClass<cls> \
		{ \
		public: \
			MetaType() : \
				MetaClassBase(super::MetaClass(), #cls, RN_FUNCTION_SIGNATURE) \
			{} \
		};

#define __RNDeclareMetaPrivateWithTraits(cls, super, ...) \
	private: \
		class MetaType : public RN::ConcreteMetaClass<cls, __VA_ARGS__> \
		{ \
		public: \
			MetaType() : \
				MetaClassBase(super::MetaClass(), #cls, RN_FUNCTION_SIGNATURE) \
			{} \
		};

#define __RNDeclareMetaPublic(cls) \
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
		RNAPI_DEFINEBASE RN::MetaClassBase *Class() const override; \
		RNAPI_DEFINEBASE static RN::MetaClassBase *MetaClass();
	
#define RNDeclareMeta(cls, super) \
	__RNDeclareMetaPrivate(cls, super) \
	__RNDeclareMetaPublic(cls)
	
#define RNDeclareMetaWithTraits(cls, super, ...) \
	__RNDeclareMetaPrivateWithTraits(cls, super, __VA_ARGS__) \
	__RNDeclareMetaPublic(cls)

#define RNDefineMeta(cls) \
	void *__kRN##cls##__metaClass = nullptr; \
	RN::MetaClassBase *cls::Class() const \
	{ \
		return cls::MetaClass(); \
	} \
	RN::MetaClassBase *cls::MetaClass() \
	{ \
		if(!__kRN##cls##__metaClass) \
			__kRN##cls##__metaClass = new cls::MetaType(); \
		return reinterpret_cast<cls::MetaType *>(__kRN##cls##__metaClass); \
	} \
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
