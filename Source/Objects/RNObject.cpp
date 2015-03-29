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

namespace RN
{
	void *__kRNObjectMetaClass = nullptr;
	
	Object::Object() :
		_refCount(1)
	{}
	
	Object::~Object()
	{
		if(!std::uncaught_exception())
			RN_ASSERT(_refCount.load() <= 1, "refCount must be <= 1 upon destructor call. Use object->Release(); instead of delete object;");
	
		for(auto &pair : _associatedObjects)
		{
			Object *object = std::get<0>(pair.second);
			MemoryPolicy policy = std::get<1>(pair.second);
			
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
		
		__DestroyWeakReferences(this);
	}
	
	void Object::InitialWakeUp(MetaClass *meta)
	{}
	
	void Object::CleanUp()
	{}
	
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
		_refCount.fetch_add(1, std::memory_order_relaxed);
		return this;
	}
	
	Object *Object::Release()
	{
		if(_refCount.fetch_sub(1, std::memory_order_release) == 1)
		{
			std::atomic_thread_fence(std::memory_order_acquire); // Synchronize all accesses to this object before deleting it

			CleanUp();
			delete this;

			return nullptr;
		}
		
		return this;
	}
	
	Object *Object::Autorelease()
	{
		AutoreleasePool *pool = AutoreleasePool::GetCurrentPool();
		if(!pool)
		{
			/*Log::Loggable loggable(Log::Level::Error);
			
			MetaClass *meta = GetClass();
			
			loggable << "Autorelease() with no pool in place, " << this << " (" << meta->GetFullname() << ") will leak!";*/
			
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
	
	
	void Object::Serialize(Serializer *serializer)
	{
		throw Exception(Exception::Type::GenericException, "Serialization not supported!");
	}
	
	
	bool Object::IsEqual(Object *other) const
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
	
	
	bool Object::IsKindOfClass(MetaClass *other) const
	{
		return GetClass()->InheritsFromClass(other);
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
		RN_ASSERT(value, "Value mustn't be NULL!");
		RN_ASSERT(key, "Key mustn't be NULL!");
		
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
				object = value->GetMetaClass()->ConstructWithCopy(value);
				break;
		}
		
		Lock();
		__RemoveAssociatedOject(key);
		
		std::tuple<Object *, MemoryPolicy> tuple = std::tuple<Object *, MemoryPolicy>(object, policy);
		_associatedObjects.insert(decltype(_associatedObjects)::value_type((void *)key, tuple));
		
		Unlock();
	}
	
	void Object::RemoveAssociatedOject(const void *key)
	{
		Lock();
		__RemoveAssociatedOject(key);
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
		RN_ASSERT(property->_object == nullptr, "ObservableProperty can only be added once to a receiver!");
		
		property->_object = this;
		property->_opaque = GetClass();
		
		_properties.push_back(property);
	}
	
	void Object::AddObservables(std::initializer_list<ObservableProperty *> properties)
	{
		_properties.reserve(_properties.size() + properties.size());
		
		for(ObservableProperty *property : properties)
		{
			RN_ASSERT(property->_object == nullptr, "ObservableProperty can only be added once to a receiver!");
			
			property->_object = this;
			property->_opaque = GetClass();
			
			_properties.push_back(property);
		}
	}
	
	void Object::MapCookie(void *cookie, ObservableProperty *property, Connection *connection)
	{
		_cookies.emplace_back(std::make_tuple(cookie, property, connection));
	}
	
	void Object::UnmapCookie(void *cookie, ObservableProperty *property)
	{
		Lock();
		
		for(auto iterator = _cookies.begin(); iterator != _cookies.end();)
		{
			auto &tuple = *iterator;
			
			if(cookie == std::get<0>(tuple) && property == std::get<1>(tuple))
			{
				std::get<2>(tuple)->Disconnect();
				
				/*if(property->_signal->GetCount() == 0)
				{
					delete property->_signal;
					property->_signal = nullptr;
				}*/
				
				iterator = _cookies.erase(iterator);
				continue;
			}
			
			iterator ++;
		}
		
		Unlock();
	}
	
	Object *Object::ResolveKeyPath(const std::string &path, std::string &key)
	{
		Object *temp = this;
		size_t offset = 0;
		
		while(1)
		{
			size_t pos = path.find('.', offset);
			
			if(pos == std::string::npos)
			{
				key = path;
				return temp;
			}
			
			temp = GetPrimitiveValueForKey(path.substr(offset, pos - offset));
			offset = pos + 1;
		}
	}
	
	Object *Object::GetPrimitiveValueForKey(const std::string &key)
	{
		for(ObservableProperty *property : _properties)
		{
			if(key.compare(property->_name) == 0)
				return property->GetValue();
		}
		
		return GetValueForUndefinedKey(key);
	}
	
	ObservableProperty *Object::GetPropertyForKeyPath(const std::string &keyPath, std::string &key)
	{
		for(ObservableProperty *property : _properties)
		{
			if(keyPath.compare(property->_name) == 0)
				return property;
		}
		
		return nullptr;
	}
	
	
	void Object::WillChangeValueForkey(const std::string &keyPath)
	{
		std::string key;
		ObservableProperty *property = GetPropertyForKeyPath(keyPath, key);
		
		if(property)
		{
			property->WillChangeValue();
			return;
		}
	}
	
	void Object::DidChangeValueForKey(const std::string &keyPath)
	{
		std::string key;
		ObservableProperty *property = GetPropertyForKeyPath(keyPath, key);
		
		if(property)
		{
			property->DidChangeValue();
			return;
		}
	}
	
	
	void Object::SetValueForKey(Object *value, const std::string &keyPath)
	{
		std::string key;
		ObservableProperty *property = GetPropertyForKeyPath(keyPath, key);
		
		property ? property->SetValue(value) : SetValueForUndefinedKey(value, key);
	}
	
	Object *Object::GetValueForKey(const std::string &keyPath)
	{
		std::string key;
		Object *object = ResolveKeyPath(keyPath, key);
		
		return object->GetPrimitiveValueForKey(key);
	}
	
	void Object::SetValueForUndefinedKey(Object *value, const std::string &key)
	{
		throw Exception(Exception::Type::InconsistencyException, "SetValue() for undefined key" + key);
	}
	
	Object *Object::GetValueForUndefinedKey(const std::string &key)
	{
		throw Exception(Exception::Type::InconsistencyException, "GetValue() for undefined key" + key);
	}
}
