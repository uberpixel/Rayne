//
//  RNAutoreleasePool.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAutoreleasePool.h"
#include "../Threads/RNThreadLocalStorage.h"

#define kRNAutoreleasePoolInitialGrowth 128
#define kRNAutoreleasePoolGrowthFactor 1.4

namespace RN
{
	static ThreadLocalStorage<AutoreleasePool *> _localPools;

	
	AutoreleasePool::AutoreleasePool() :
		_parent(AutoreleasePool::GetCurrentPool()),
		_owner(std::this_thread::get_id())
	{
		_objects.reserve(kRNAutoreleasePoolInitialGrowth);
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

		//TODO: Maybe delete and remove this as friend of Object
#if RN_BUILD_DEBUG
		int counter = 0;
		for(const Object *obj : _objects)
		{
			if(obj == object)
			{
				counter += 1;
				RN_ASSERT(counter < object->_refCount, "Object will be overreleased by autorelease pool!");
			}
		}
#endif

		_objects.push_back(object);
		
		if(_objects.size() == _objects.capacity())
			_objects.reserve(static_cast<size_t>(_objects.size() + (_objects.size() * kRNAutoreleasePoolGrowthFactor)));
	}
	
	void AutoreleasePool::Drain()
	{
		std::vector<const Object *> objects;
		std::swap(objects, _objects);

		for(const Object *object : objects)
		{
			object->Release();
		}

		_objects = std::vector<const Object *>();
		_objects.reserve(kRNAutoreleasePoolInitialGrowth);
	}
	
	AutoreleasePool *AutoreleasePool::GetCurrentPool()
	{
		return _localPools.GetValue();
	}
}
