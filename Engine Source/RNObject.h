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
		
		RNAPI virtual bool IsEqual(Object *other) const;
		RNAPI virtual machine_hash Hash() const;
		RNAPI bool IsKindOfClass(MetaClass *other) const;
		RNAPI bool IsMemberOfClass(MetaClass *other) const;
		
		RNAPI virtual void Serialize(Serializer *serializer);
		
		virtual class MetaClass *Class() const
		{
			return Object::__metaClass;
		}
		static class MetaClass *MetaClass()
		{
			if(!__metaClass)
				__metaClass = new MetaType();
			
			return __metaClass;
		}
		
		enum class MemoryPolicy
		{
			Assign,
			Retain,
			Copy
		};
		
		void SetAssociatedObject(const void *key, Object *value, MemoryPolicy policy);
		void RemoveAssociatedOject(const void *key);
		Object *AssociatedObject(const void *key);
		
	private:
		class MetaType : public ConcreteMetaClass<Object>
		{
		public:
			MetaType() :
				MetaClass(0, "Object", __PRETTY_FUNCTION__)
			{}
		};
		
		void __RemoveAssociatedOject(const void *key);
		
		static MetaType *__metaClass;
		SpinLock _lock;
		
		std::atomic<machine_uint> _refCount;
		std::unordered_map<void *, std::tuple<Object *, MemoryPolicy>> _associatedObjects;
	};
	
#define __RNDefineMetaPrivate(cls, super) \
	private: \
		class MetaType : public RN::ConcreteMetaClass<cls> \
		{ \
		public: \
			MetaType() : \
				MetaClass(super::MetaClass(), #cls, __PRETTY_FUNCTION__) \
			{} \
		}; \
		static MetaType *__##cls##__metaClass; \

#define __RNDefineMetaPrivateWithTraits(cls, super, ...) \
	private: \
		class MetaType : public RN::ConcreteMetaClass<cls, __VA_ARGS__> \
		{ \
		public: \
			MetaType() : \
				MetaClass(super::MetaClass(), #cls, __PRETTY_FUNCTION__) \
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
		class RN::MetaClass *Class() const override \
		{ \
			return cls::__##cls##__metaClass; \
		} \
		static class RN::MetaClass *MetaClass() \
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
	}
}

#endif /* __RAYNE_OBJECT_H__ */
