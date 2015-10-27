//
//  RNAutoreleasePool.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAutoreleasePool.h"
#include "../Threads/RNThreadLocalStorage.h"

#define kRNAutoreleasePoolGrowthRate 128
#define RNDebugAutoreleasePools 0

namespace RN
{
	static ThreadLocalStorage<AutoreleasePool *> _localPools;

	struct AutoreleasePoolInternals
	{
		std::thread::id owner;
		
#if RNDebugAutoreleasePools
		std::vector<std::pair<Object *, Exception>> objects;
#else 
		std::vector<Object *> objects;
#endif
	};
	
	AutoreleasePool::AutoreleasePool() :
		_parent(AutoreleasePool::GetCurrentPool())
	{
		_internals->owner = std::this_thread::get_id();
		_internals->objects.reserve(kRNAutoreleasePoolGrowthRate);

		_localPools.SetValue(this);
	}
	
	AutoreleasePool::~AutoreleasePool()
	{
		RN_ASSERT(this == GetCurrentPool(), "Popping pool other than the topmost pool is forbidden!");
		
		Drain();
		_localPools.SetValue(_parent);
	}

	void AutoreleasePool::PerformBlock(Function &&function)
	{
		AutoreleasePool pool;
		function();
		pool.Drain();
	}

	void AutoreleasePool::AddObject(Object *object)
	{
#if RNDebugAutoreleasePools
		_internals->objects.emplace_back(std::make_pair(object, Exception(Exception::Type::GenericException, "Traceback")));
#else
		_internals->objects.push_back(object);
#endif
		
		if((_internals->objects.size() % kRNAutoreleasePoolGrowthRate) == 0)
			_internals->objects.reserve(_internals->objects.size() + kRNAutoreleasePoolGrowthRate);
	}
	
	void AutoreleasePool::Drain()
	{
#if RNDebugAutoreleasePools
		for(auto &pair : _internals->objects)
		{
			Object *object = pair.first;
			object->Release();
			
		}
#else
		for(Object *object : _internals->objects)
		{
			object->Release();
		}
#endif
		
		_internals->objects.clear();
	}
	
	AutoreleasePool *AutoreleasePool::GetCurrentPool()
	{
		return _localPools.GetValue();
	}
}
