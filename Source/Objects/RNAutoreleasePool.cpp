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

namespace RN
{
	static ThreadLocalStorage<AutoreleasePool *> _localPools;

	struct AutoreleasePoolInternals
	{
		std::thread::id owner;
		std::vector<const Object *> objects;
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

	void AutoreleasePool::AddObject(const Object *object)
	{
		_internals->objects.push_back(object);
		
		if((_internals->objects.size() % kRNAutoreleasePoolGrowthRate) == 0)
			_internals->objects.reserve(_internals->objects.size() + kRNAutoreleasePoolGrowthRate);
	}
	
	void AutoreleasePool::Drain()
	{
		for(auto iterator = _internals->objects.begin(); iterator != _internals->objects.end(); iterator++)
		{
			(*iterator)->Release();
		}
		
		_internals->objects.clear();
	}
	
	AutoreleasePool *AutoreleasePool::GetCurrentPool()
	{
		return _localPools.GetValue();
	}
}
