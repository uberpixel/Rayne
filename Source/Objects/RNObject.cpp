//
//  RNObject.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNObject.h"
#include "RNAutoreleasePool.h"
#include "RNData.h"
#include "RNObjectInternals.h"
#include "RNString.h"
#include "../Debug/RNLogger.h"
#include <string>

namespace RN
{
	void *__kRNObjectMetaClass = nullptr;
	
	Object::Object() :
		_refCount(1)
	{}
	
	Object::~Object()
	{
		if(!std::uncaught_exception())
			RN_ASSERT(_refCount.load(std::memory_order_relaxed) <= 1, "refCount must be <= 1 upon destructor call. Use object->Unlock(); instead of delete object;");
	
		for(auto &pair : _associatedObjects)
		{
			MemoryPolicy policy = std::get<1>(pair.second);
			
			switch(policy)
			{
				case MemoryPolicy::Retain:
				case MemoryPolicy::Copy:
				{
					Object *object = std::get<0>(pair.second);
					object->Release();
					break;
				}
					
				default:
					break;
			}
		}
		
		__DestroyWeakReferences(this);
	}
	
	void Object::InitialWakeUp(MetaClass *meta)
	{}
	
	void Object::Dealloc()
	{}

	const String *Object::GetDescription() const
	{
		return RNSTR("<" << GetClass()->GetFullname() << ":" << (void *)this << ">");
	}
	
	MetaClass *Object::GetClass() const
	{
		return Object::GetMetaClass();
	}
	MetaClass *Object::GetMetaClass()
	{
		if(!__kRNObjectMetaClass)
		{
			__InitWeakTables();
			__kRNObjectMetaClass = new MetaType();
		}
		
		return reinterpret_cast<Object::MetaType *>(__kRNObjectMetaClass);
	}
	
	Object *Object::Retain()
	{
		_refCount.fetch_add(1, std::memory_order_relaxed); // RMW pairs with relaxed memory ordering
		return this;
	}
	const Object *Object::Retain() const
	{
		_refCount.fetch_add(1, std::memory_order_relaxed); // RMW pairs with relaxed memory ordering
		return this;
	}
	

	void Object::Release() const
	{
		// If this is the last reference this thread has, which it very well might be,
		// we need to flush all accesses done so far. Thus the release barrier
		if(_refCount.fetch_sub(1, std::memory_order_release) == 1)
		{
			// Catch up with all changes from all other threads that had access to the object
			std::atomic_thread_fence(std::memory_order_acquire);

			// Since this function is marked const, we need to do this
			Object *nonConstThis = const_cast<Object *>(this);

			nonConstThis->Dealloc();
			delete nonConstThis;
		}
	}
	
	Object *Object::Autorelease()
	{
		AutoreleasePool *pool = AutoreleasePool::GetCurrentPool();
		if(!pool)
		{
			AutoreleasePool::PerformBlock([this]() {

				MetaClass *meta = GetClass();
				RNError("Autorelease() with no pool in place, <" << meta->GetFullname() << ":" << reinterpret_cast<void *>(this) << "> will leak!");

			});

			return this;
		}

		pool->AddObject(this);
		return this;
	}
	const Object *Object::Autorelease() const
	{
		AutoreleasePool *pool = AutoreleasePool::GetCurrentPool();
		if(!pool)
		{
			AutoreleasePool::PerformBlock([this]() {

				MetaClass *meta = GetClass();
				RNError("Autorelease() with no pool in place, <" << meta->GetFullname() << ":" << reinterpret_cast<const void *>(this) << "> will leak!");

			});

			return this;
		}

		pool->AddObject(this);
		return this;
	}
	
	
	Object *Object::Copy() const
	{
		RN_ASSERT(GetClass()->SupportsCopying(), "Only Objects that support the copy trait can be copied!\n");
		return GetClass()->ConstructWithCopy(const_cast<Object *>(this));
	}
	
	
	void Object::Serialize(Serializer *serializer) const
	{
		throw InconsistencyException("Serialization not supported (or a subclass called Object::Serialize)");
	}
	
	
	bool Object::IsEqual(const Object *other) const
	{
		return (this == other);
	}
	
	size_t Object::GetHash() const
	{
		size_t hash = reinterpret_cast<size_t>(this);
		
		hash = ~hash + (hash << 15);
		hash = hash ^ (hash >> 12);
		hash = hash + (hash << 2);
		hash = hash ^ (hash >> 4);
		hash = hash * 2057;
		hash = hash ^ (hash >> 16);
		
		return hash;
	}
	
	
	bool Object::IsKindOfClass(const MetaClass *other) const
	{
		return GetClass()->InheritsFromClass(other);
	}
	
	
	
	void Object::__RemoveAssociatedObject(const void *key)
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
		RN_ASSERT(value, "Value mustn't be NULL!");
		RN_ASSERT(key, "Key mustn't be NULL!");
		
		Object *object = nullptr;

		if(value)
		{
			switch(policy)
			{
				case MemoryPolicy::Assign:
					object = value;
					break;

				case MemoryPolicy::Retain:
					object = value->Retain();
					break;

				case MemoryPolicy::Copy:
					object = value->GetMetaClass()->ConstructWithCopy(value);
					break;
			}
		}
		
		Lock();
		__RemoveAssociatedObject(key);

		if(object)
		{
			std::tuple<Object *, MemoryPolicy> tuple = std::make_tuple(object, policy);
			_associatedObjects.emplace(const_cast<void *>(key), std::move(tuple));
		}

		Unlock();
	}
	
	Object *Object::GetAssociatedObject(const void *key)
	{
		Object *object = 0;
		
		Lock();
		
		auto iterator = _associatedObjects.find((void *)key);
		
		if(iterator != _associatedObjects.end())
			object = std::get<0>(iterator->second);
		
		Unlock();
		
		return object;			
	}
	
	
	void Object::Lock()
	{
		_lock.Lock();
	}
	
	void Object::Unlock()
	{
		_lock.Unlock();
	}
	
	// ---------------------
	// MARK: -
	// MARK: KVO / KVC
	// ---------------------
	
	std::vector<ObservableProperty *> Object::GetPropertiesForClass(MetaClass *meta)
	{
		if(!meta)
			return _properties;
		
		std::vector<ObservableProperty *> result;
		
		for(ObservableProperty *property : _properties)
		{
			if(property->_opaque == meta)
				result.push_back(property);
		}
		
		return result;
	}
	
	void Object::AddObservable(ObservableProperty *property)
	{
		RN_ASSERT(property->_owner == nullptr, "ObservableProperty can only be added once to a receiver!");
		
		property->_owner = this;
		property->_opaque = GetClass();
		
		_properties.push_back(property);
	}
	
	void Object::AddObservables(const std::initializer_list<ObservableProperty *> properties)
	{
		_properties.reserve(_properties.size() + properties.size());
		
		for(ObservableProperty *property : properties)
			AddObservable(property);
	}
	
	void Object::MapCookie(void *cookie, ObservableProperty *property, Connection *connection) const
	{
		LockGuard<RecursiveLockable> lock(const_cast<RecursiveLockable &>(_lock));
		_cookies.emplace_back(std::make_tuple(cookie, property, connection));
	}
	
	void Object::UnmapCookie(void *cookie, ObservableProperty *property) const
	{
		LockGuard<RecursiveLockable> lock(const_cast<RecursiveLockable &>(_lock));
		
		for(auto iterator = _cookies.begin(); iterator != _cookies.end();)
		{
			auto &tuple = *iterator;
			
			if(cookie == std::get<0>(tuple) && property == std::get<1>(tuple))
			{
				std::get<2>(tuple)->Disconnect();
				
				if(property->_signal && property->_signal->GetCount() == 0)
				{
					delete property->_signal;
					property->_signal = nullptr;
				}
				
				iterator = _cookies.erase(iterator);
				continue;
			}
			
			iterator ++;
		}
	}

	ObservableProperty *Object::GetPropertyForKey(const char *key) const
	{
		LockGuard<RecursiveLockable> lock(const_cast<RecursiveLockable &>(_lock));

		for(ObservableProperty *property : _properties)
		{
			if(strcmp(property->_name, key) == 0)
				return property;
		}

		return nullptr;
	}

	
	Object *Object::GetPrimitiveValueForKey(const char *key) const
	{
		for(ObservableProperty *property : _properties)
		{
			if(strcmp(key, property->_name) == 0)
				return property->GetValue();
		}
		
		return GetValueForUndefinedKey(key);
	}

	Object *Object::GetPrimitiveValueForKeyPath(const char *keyPath) const
	{
		char storage[32];
		const char *temp = strchr(keyPath, '.');

		if(!temp)
			return GetPrimitiveValueForKey(keyPath);

		temp ++;

#if RN_PLATFORM_MAC_OS
		strlcpy(storage, keyPath, temp - keyPath);
#else
		strncpy(storage, keyPath, temp - keyPath);
		storage[32] = '\0';
#endif

		Object *next = GetValueForKey(storage);
		if(!next)
			return GetValueForUndefinedKey(storage);

		return next->GetPrimitiveValueForKeyPath(temp);
	}


	void Object::WillChangeValueForKey(const char *key)
	{
		ObservableProperty *property = GetPropertyForKey(key);
		if(property)
		{
			property->WillChangeValue();
			return;
		}
	}
	
	void Object::DidChangeValueForKey(const char *key)
	{
		ObservableProperty *property = GetPropertyForKey(key);
		
		if(property)
		{
			property->DidChangeValue();
			return;
		}
	}


	void Object::SetValueForKeyPath(Object *value, const char *keyPath)
	{
		char storage[32];
		const char *temp = strchr(keyPath, '.');

		if(!temp)
		{
			SetValueForKey(value, keyPath);
			return;
		}

		temp ++;

#if RN_PLATFORM_MAC_OS
		strlcpy(storage, keyPath, temp - keyPath);
#else
		strncpy(storage, keyPath, temp - keyPath);
		storage[32] = '\0';
#endif

		Object *next = GetValueForKey(storage);
		if(!next)
			throw InconsistencyException(RNSTR("SetValueForKeyPath() with undefined key '" << storage << "' on " << this));

		next->SetValueForKeyPath(value, temp);
	}

	void Object::SetValueForKey(Object *value, const char *key)
	{
		ObservableProperty *property = GetPropertyForKey(key);
		property ? property->SetValue(value) : SetValueForUndefinedKey(value, key);
	}

	void Object::SetPrimitiveValueForKey(Object *value, const char *key)
	{
		for(ObservableProperty *property : _properties)
		{
			if(strcmp(key, property->_name) == 0)
				property->SetValue(value);
		}

		SetValueForUndefinedKey(value, key);
	}


	void Object::SetValueForUndefinedKey(Object *value, const char *key)
	{
		throw InconsistencyException(RNSTR("SetValueForKey() with undefined key '" << key << "'"));
	}
	
	Object *Object::GetValueForUndefinedKey(const char *key) const
	{
		throw InconsistencyException(RNSTR("GetValueForKey() with undefined key '" << key << "'"));
	}
}
