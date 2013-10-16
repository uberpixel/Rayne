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
#include "RNKVO.h"

namespace RN
{
	class Serializer;
	class Object : public ObservableContainer
	{
	public:
		RNAPI Object();
		RNAPI ~Object() override;
		
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
		
		enum class MemoryPolicy
		{
			Assign,
			Retain,
			Copy
		};
		
		void SetAssociatedObject(const void *key, Object *value, MemoryPolicy policy);
		void RemoveAssociatedOject(const void *key);
		Object *GetAssociatedObject(const void *key);
		
	protected:
		virtual void CleanUp();
		
	private:
		class MetaType : public ConcreteMetaClass<Object>
		{
		public:
			MetaType() :
				MetaClassBase(0, "Object", __PRETTY_FUNCTION__)
			{}
		};
		
		void __RemoveAssociatedOject(const void *key);
		
		static MetaType *__metaClass;
		std::mutex _lock;
		
		std::atomic<size_t> _refCount;
		std::atomic_flag _cleanUpFlag;
		std::unordered_map<void *, std::tuple<Object *, MemoryPolicy>> _associatedObjects;
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
	cls::MetaType *cls::__##cls##__metaClass = 0; \
	void __##cls##__load() __attribute((constructor)); \
	void __##cls##__load() \
	{ \
		cls::MetaClass(); \
		cls::InitialWakeUp(cls::MetaClass()); \
	}
	
	template<class T>
	static void SafeRelease(T *&object)
	{
		if(object)
		{
			object->Release();
			object = nullptr;
			
			return;
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
