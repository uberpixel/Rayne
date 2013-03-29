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
		
	private:
		class MetaType : public MetaClass
		{
		public:
			virtual Object *Construct()
			{
				return new Object();
			}
			
			MetaType() :
				MetaClass(0, "Object")
			{}
		};
		
		SpinLock _lock;
		machine_int _refCount;
		static MetaType *__metaClass;
	};
	
#define __RNDefineMetaPrivate(cls, super, cnstr) \
	private: \
		class MetaType : public MetaClass \
		{ \
		public: \
			MetaType() : \
				MetaClass(super::MetaClass(), #cls) \
			{} \
			virtual cls *Construct() \
			{ \
				return cnstr; \
			} \
		}; \
		static MetaType *__##cls##__metaClass; \
	
#define __RNDefineMetaPublic(cls) \
	public: \
		virtual class MetaClass *Class() const \
		{ \
			return cls::__##cls##__metaClass; \
		} \
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
		static class MetaClass *MetaClass() \
		{ \
			if(!cls::__##cls##__metaClass) \
				cls::__##cls##__metaClass = new cls::MetaType(); \
			return cls::__##cls##__metaClass; \
		}
	
#define RNDefineMeta(cls, super) \
	__RNDefineMetaPrivate(cls, super, new cls()) \
	__RNDefineMetaPublic(cls)
	
	
#define RNDefineConstructorlessMeta(cls, super) \
	__RNDefineMetaPrivate(cls, super, 0) \
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
