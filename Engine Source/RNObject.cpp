//
//  RNObject.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		RN_ASSERT(_refCount.load() <= 1, "refCount must be <= 1 upon destructor call. Use object->Release(); instead of delete object;");
	
		for(auto i=_associatedObjects.begin(); i!=_associatedObjects.end(); i++)
		{
			Object *object = std::get<0>(i->second);
			MemoryPolicy policy = std::get<1>(i->second);
			
			switch(policy)
			{
				case MemoryPolicy::Retain:
				case MemoryPolicy::Copy:
					object->Release();
					break;
					
				default:
					break;
			}
		}
	}
	
	
	Object *Object::Retain()
	{
		_refCount ++;
		return this;
	}
	
	Object *Object::Release()
	{
		if(_refCount.fetch_sub(1) == 1)
		{
			delete this;
			return 0;
		}
		
		return this;
	}
	
	Object *Object::Autorelease()
	{
		AutoreleasePool *pool = AutoreleasePool::CurrentPool();
		if(!pool)
		{
			printf("Autorelease() with no pool in place, %p will leak!\n", this);
			return this;
		}
		
		pool->AddObject(this);
		return this;
	}
	
	
	void Object::Serialize(Serializer *serializer)
	{
		throw Exception(Exception::Type::GenericException, "Serialization not supported!");
	}
	
	
	bool Object::IsEqual(Object *other) const
	{
		return (this == other);
	}
	
	machine_hash Object::Hash() const
	{
		machine_hash hash = (machine_hash)this;
		
		hash = ~hash + (hash << 15);
		hash = hash ^ (hash >> 12);
		hash = hash + (hash << 2);
		hash = hash ^ (hash >> 4);
		hash = hash * 2057;
		hash = hash ^ (hash >> 16);
		
		return hash;
	}
	
	
	bool Object::IsKindOfClass(class MetaClass *other) const
	{
		return Class()->InheritsFromClass(other);
	}
	
	bool Object::IsMemberOfClass(class MetaClass *other) const
	{
		return (Class() == other);
	}
	
	
	
	void Object::__RemoveAssociatedOject(const void *key)
	{
		auto iterator = _associatedObjects.find((void *)key);
		if(iterator != _associatedObjects.end())
		{
			Object *object = std::get<0>(iterator->second);
			MemoryPolicy policy = std::get<1>(iterator->second);
			
			switch(policy)
			{
				case MemoryPolicy::Retain:
				case MemoryPolicy::Copy:
					object->Release();
					break;
					
				default:
					break;
			}
			
			_associatedObjects.erase(iterator);
		}
	}
	
	
	void Object::SetAssociatedObject(const void *key, Object *value, MemoryPolicy policy)
	{
		RN_ASSERT0(value);
		RN_ASSERT0(key);
		
		Object *object = 0;
		
		switch(policy)
		{
			case MemoryPolicy::Assign:
				object = value;
				break;
				
			case MemoryPolicy::Retain:
				object = value->Retain();
				break;
				
			case MemoryPolicy::Copy:
				break;
		}
		
		_lock.Lock();
		__RemoveAssociatedOject(key);
		
		std::tuple<Object *, MemoryPolicy> tuple = std::tuple<Object *, MemoryPolicy>(object, policy);
		_associatedObjects.insert(std::unordered_map<void *, std::tuple<Object *, MemoryPolicy>>::value_type((void *)key, tuple));
		
		_lock.Unlock();
	}
	
	void Object::RemoveAssociatedOject(const void *key)
	{
		_lock.Lock();
		__RemoveAssociatedOject(key);
		_lock.Unlock();
	}
	
	Object *Object::AssociatedObject(const void *key)
	{
		Object *object = 0;
		
		_lock.Lock();
		
		auto iterator = _associatedObjects.find((void *)key);
		
		if(iterator != _associatedObjects.end())
			object = std::get<0>(iterator->second);
		
		_lock.Unlock();
		
		return object;			
	}
}
