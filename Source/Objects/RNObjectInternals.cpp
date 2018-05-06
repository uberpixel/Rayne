//
//  RNObjectInternals.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNObjectInternals.h"
#include "RNObject.h"
#include <cstdint>

#if RN_PLATFORM_MAC_OS
#include <sys/sysctl.h>
#endif
#if RN_PLATFORM_LINUX
#include <unistd.h>
#endif

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

	size_t GetCachelineSize()
	{
#if RN_PLATFORM_MAC_OS
		size_t cacheLineSize = 0;
		size_t sizeofSizeT = sizeof(size_t);

		sysctlbyname("hw.cachelinesize", &cacheLineSize, &sizeofSizeT, 0, 0);
#endif

#if RN_PLATFORM_WINDOWS
		size_t cacheLineSize = 0;
		DWORD bufferSize = 0;
		SYSTEM_LOGICAL_PROCESSOR_INFORMATION *buffer = 0;

		::GetLogicalProcessorInformation(0, &bufferSize);
		buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(bufferSize);

		::GetLogicalProcessorInformation(&buffer[0], &bufferSize);

		for(DWORD i = 0; i != bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i)
		{
			if(buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == 1)
			{
				cacheLineSize = buffer[i].Cache.LineSize;
				break;
			}
		}

		free(buffer);
#endif

#if RN_PLATFORM_LINUX
		size_t cacheLineSize = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
#endif

#if RN_PLATFORM_ANDROID
		size_t cacheLineSize = 128; //Best guess as used by SDL2...
#endif

		return cacheLineSize; /* Leave here to trigger an error on all unsupported platforms */
	}

	static size_t __CacheLineSize;
	static size_t __WeakBucketSpread;
	
	class WeakTable
	{
	public:
		static WeakTable *GetWeakTableForPointer(void *pointer)
		{
			std::uintptr_t ptr = reinterpret_cast<uintptr_t>(pointer);
			size_t index = ((ptr >> 4) ^ (ptr >> 9)) & (kRNWeakBucketCount - 1);
			
			return reinterpret_cast<WeakTable *>(&__WeakBuckets[index * __WeakBucketSpread]);
		}
		static void Init()
		{
			__WeakBuckets = reinterpret_cast<uint8 *>(Memory::AllocateAligned(kRNWeakBucketCount * __WeakBucketSpread, __CacheLineSize));
			
			for(size_t i = 0; i < kRNWeakBucketCount; i ++)
				new (&__WeakBuckets[i * __WeakBucketSpread]) WeakTable();
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
		
		
		Lockable lock;
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
		
		Lockable &lock1 = (oldTable > newTable) ? newTable->lock : oldTable->lock;
		Lockable &lock2 = (oldTable > newTable) ? oldTable->lock : newTable->lock;
		
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
		Lockable &lock = table->lock;
		
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
		Lockable &lock = table->lock;
		
		lock.Lock();
		
		if(object != *weak) // The object changed while we were locking
		{
			lock.Unlock();
			goto retry;
		}
		
		// Load the weak reference
		object = table->RemoveWeakReference(object);
		*weak  = nullptr;
		
		(void)(object);
		
		lock.Unlock();
		
		return nullptr;
	}
	
	
	// Private API
	
	void __InitWeakTables()
	{
		__CacheLineSize = std::min(GetCachelineSize(), static_cast<size_t>(64));
		__WeakBucketSpread = __CacheLineSize * 2;

		WeakTable::Init();
	}
	
	void __DestroyWeakReferences(Object *object)
	{
		WeakTable *table = WeakTable::GetWeakTableForPointer(object);
		Lockable &lock = table->lock;
		
		lock.Lock();
		table->DestroyWeakReference(object);
		lock.Unlock();
	}
}
