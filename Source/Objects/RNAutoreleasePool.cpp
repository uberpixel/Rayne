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

	
	AutoreleasePool::AutoreleasePool() :
		_parent(AutoreleasePool::GetCurrentPool()),
		_owner(std::this_thread::get_id())
	{
		_objects.reserve(kRNAutoreleasePoolGrowthRate);
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
		RN_ASSERT(object, "Object mustn't be NULL");

		_objects.push_back(object);
		
		if((_objects.size() % kRNAutoreleasePoolGrowthRate) == 0)
			_objects.reserve(_objects.size() + kRNAutoreleasePoolGrowthRate);
	}
	
	void AutoreleasePool::Drain()
	{
		for(auto iterator = _objects.begin(); iterator != _objects.end(); iterator++)
			(*iterator)->Release();
		
		_objects.clear();
	}
	
	AutoreleasePool *AutoreleasePool::GetCurrentPool()
	{
		return _localPools.GetValue();
	}
}
