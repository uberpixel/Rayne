//
//  RNObject.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNObject.h"
#include "RNAutoreleasePool.h"

namespace RN
{
	Object::MetaType *Object::__metaClass = 0;
	
	Object::Object()
	{
		_refCount = 1;
	}
	
	Object::~Object()
	{
		//class MetaClass *meta = Class(); // Just there so that it shows up on the stack
		RN_ASSERT(_refCount <= 1, "refCount must be <= 1 upon destructor call. Use object->Release(); instead of delete object;");
	}
	
	
	Object *Object::Retain()
	{
		if(!this)
			return this;
		
		_lock.Lock();
		_refCount ++;
		_lock.Unlock();
		
		return this;
	}
	
	Object *Object::Release()
	{
		if(!this)
			return this;
		
		_lock.Lock();
		
		if((-- _refCount) == 0)
		{
			_lock.Unlock();
			
			delete this;
			return 0;
		}
		
		_lock.Unlock();
		return this;
	}
	
	Object *Object::Autorelease()
	{
		if(!this)
			return this;
		
		AutoreleasePool *pool = AutoreleasePool::CurrentPool();
		if(!pool)
		{
			printf("Autorelease() with no pool in place, %p will leak!\n", this);
			return this;
		}
		
		pool->AddObject(this);
		return this;
	}
	
	
	
	bool Object::IsEqual(Object *other) const
	{
		return (this == other);
	}
	
	machine_hash Object::Hash() const
	{
		return (machine_hash)this;
	}
	
	
	bool Object::IsKindOfClass(class MetaClass *other) const
	{
		return Class()->InheritsFromClass(other);
	}
	
	bool Object::IsMemberOfClass(class MetaClass *other) const
	{
		return (Class() == other);
	}
}
