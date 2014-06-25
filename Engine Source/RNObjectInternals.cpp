//
//  RNObjectInternals.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNObjectInternals.h"
#include "RNObject.h"
#include <cstdint>

#define kRNCPUCacheLineMax 64

#define kRNWeakBucketsSpread (kRNCPUCacheLineMax) * 2
#define kRNWeakBucketCount   8

namespace RN
{
	// Have the buckets spread out into different cache lines to avoid false sharing locks
	static uint8 *__WeakBuckets;
	
	struct WeakHash
	{
		size_t operator()(const void *pointer) const
		{
			std::uintptr_t hash = reinterpret_cast<std::uintptr_t>(pointer);
			
			hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
			hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
			hash = ((hash >> 16) ^ hash);
			
			return hash;
		}
	};
	
	struct WeakEntry
	{
		WeakEntry(Object *tobject) :
			object(tobject),
			references(1)
		{}
		
		Object *object;
		size_t references;
	};
	
	class WeakTable
	{
	public:
		static WeakTable *GetWeakTableForPointer(void *pointer)
		{
			std::uintptr_t ptr = (std::uintptr_t)pointer;
			int index = ((ptr >> 4) ^ (ptr >> 9)) & (kRNWeakBucketCount - 1);
			
			return reinterpret_cast<WeakTable *>(&__WeakBuckets[index * kRNWeakBucketsSpread]);
		}
		static void Init()
		{
			__WeakBuckets = new uint8[kRNWeakBucketCount * kRNWeakBucketsSpread];
			
			for(size_t i = 0; i < kRNWeakBucketCount; i ++)
				new (&__WeakBuckets[i * kRNWeakBucketsSpread]) WeakTable();
		}
		
		
		// MUST be locked to perform any of the following operations
		Object *InsertWeakReference(Object *reference)
		{
			if(!reference)
				return nullptr;
			
			auto iterator = weakMap.find(reference);
			if(iterator != weakMap.end())
			{
				iterator->second->references ++;
				return reference;
			}
			
			weakMap.emplace(reference, new WeakEntry(reference));
			return reference;
		}
		
		Object *LookupWeakReference(Object *reference)
		{
			if(!reference)
				return nullptr;
			
			auto iterator = weakMap.find(reference);
			if(iterator != weakMap.end())
				return reference;
			
			return nullptr;
		}
		
		Object *RemoveWeakReference(Object *reference)
		{
			auto iterator = weakMap.find(reference);
			if(iterator != weakMap.end())
			{
				WeakEntry *entry = iterator->second;
				
				if((-- entry->references) == 0)
				{
					delete entry;
					weakMap.erase(iterator);
					
					return nullptr;
				}
				
				return reference;
			}
			
			return nullptr;
		}
		
		void DestroyWeakReference(Object *reference)
		{
			auto iterator = weakMap.find(reference);
			if(iterator != weakMap.end())
			{
				WeakEntry *entry = iterator->second;
				
				delete entry;
				weakMap.erase(iterator);
			}
		}
		
		
		SpinLock lock;
		std::unordered_map<void *, WeakEntry *, WeakHash> weakMap;
	};
	
	
	
	
	// "Public" accessor functions that take care of working with the weak table
	
	Object *__InitWeak(Object **weak, Object *value)
	{
		*weak = nullptr;
		
		if(!value)
			return nullptr;
		
		return __StoreWeak(weak, value);
	}
	
	Object *__StoreWeak(Object **weak, Object *value)
	{
	retry:
		Object *object = *weak;
		
		WeakTable *oldTable = WeakTable::GetWeakTableForPointer(object);
		WeakTable *newTable = WeakTable::GetWeakTableForPointer(value);
		
		SpinLock &lock1 = (oldTable > newTable) ? newTable->lock : oldTable->lock;
		SpinLock &lock2 = (oldTable > newTable) ? oldTable->lock : newTable->lock;
		
		bool differentLocks = (oldTable != newTable);
		if(differentLocks)
			lock2.Lock();
		
		lock1.Lock();
		
		if(object != *weak) // The object changed while we were locking
		{
			lock1.Unlock();
			
			if(differentLocks)
				lock2.Unlock();
			
			goto retry;
		}
		
		// Store the new weak reference and delete the old one
		Object *newObject = newTable->InsertWeakReference(value);
		oldTable->RemoveWeakReference(object);
		
		// Update the weak pointer while locked
		*weak = newObject;
		
		lock1.Unlock();
		
		if(differentLocks)
			lock2.Unlock();
		
		return newObject;
	}
	
	Object *__LoadWeakObjectRetained(Object **weak)
	{
	retry:
		Object *object = *weak;
		
		WeakTable *table = WeakTable::GetWeakTableForPointer(object);
		SpinLock &lock = table->lock;
		
		lock.Lock();
		
		if(object != *weak) // The object changed while we were locking
		{
			lock.Unlock();
			goto retry;
		}
		
		// Load the weak reference
		object = table->LookupWeakReference(object);
		*weak = object;
		
		if(object)
			object->Retain();
		
		lock.Unlock();
		return object;
	}
	
	Object *__RemoveWeakObject(Object **weak)
	{
	retry:
		Object *object = *weak;
		
		WeakTable *table = WeakTable::GetWeakTableForPointer(object);
		SpinLock &lock = table->lock;
		
		lock.Lock();
		
		if(object != *weak) // The object changed while we were locking
		{
			lock.Unlock();
			goto retry;
		}
		
		// Load the weak reference
		object = table->RemoveWeakReference(object);
		*weak  = nullptr;
		
		lock.Unlock();
		
		return nullptr;
	}
	
	
	// Private API
	
	void __InitWeakTables()
	{
		WeakTable::Init();
	}
	
	void __DestroyWeakReferences(Object *object)
	{
		WeakTable *table = WeakTable::GetWeakTableForPointer(object);
		SpinLock &lock = table->lock;
		
		lock.Lock();
		table->DestroyWeakReference(object);
		lock.Unlock();
	}
}
