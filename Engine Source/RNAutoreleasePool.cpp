//
//  RNAutoreleasePool.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAutoreleasePool.h"
#include "RNThreadLocalStorage.h"

#define kRNAutoreleasePoolGrowthRate 128

namespace RN
{
	static stl::thread_local_storage<AutoreleasePool *> _local_pools;
	
	AutoreleasePool::AutoreleasePool() :
		_owner(std::this_thread::get_id()),
		_parent(AutoreleasePool::GetCurrentPool())
	{
		_objects.reserve(kRNAutoreleasePoolGrowthRate);
		_local_pools.get() = this;
	}
	
	AutoreleasePool::~AutoreleasePool()
	{
		RN_ASSERT(this == GetCurrentPool(), "Popping pool other than the topmost pool is forbidden!");
		
		Drain();
		_local_pools.get() = _parent;
	}
	
	void AutoreleasePool::AddObject(Object *object)
	{
		_objects.push_back(object);
		
		if((_objects.size() % kRNAutoreleasePoolGrowthRate) == 0)
			_objects.reserve(_objects.size() + kRNAutoreleasePoolGrowthRate);
	}
	
	void AutoreleasePool::Drain()
	{
		for(auto i=_objects.begin(); i!=_objects.end(); i++)
		{
			Object *object = *i;
			object->Release();
		}
		
		_objects.clear();
	}
	
	AutoreleasePool *AutoreleasePool::GetCurrentPool()
	{
		return _local_pools.get();
	}
}
