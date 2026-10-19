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

#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
		RN_ASSERT(object->_availableReferenceCount.fetch_sub(1, std::memory_order_relaxed) > 0, "Object will be overreleased by autorelease pool!");
		object->_autoreleaseCounter.fetch_add(1, std::memory_order_relaxed);
#endif

		_objects.push_back(object);

		if(_objects.size() == _objects.capacity())
			_objects.reserve(static_cast<size_t>(_objects.size() + (_objects.size() * kRNAutoreleasePoolGrowthFactor)));
	}

	void AutoreleasePool::Drain()
	{
		for(size_t i = 0; i < _objects.size(); i++)
		{
			const Object *object = _objects[i];
#if RN_BUILD_DEBUG || RN_BUILD_DEBUG_REFCOUNT
			object->_autoreleaseCounter.fetch_sub(1, std::memory_order_relaxed);
			// Return the pool reservation for the Release() below to consume.
			object->_availableReferenceCount.fetch_add(1, std::memory_order_relaxed);
#endif
			object->Release();
		}

		_objects = std::vector<const Object *>();
		_objects.reserve(kRNAutoreleasePoolInitialGrowth);
	}

	AutoreleasePool *AutoreleasePool::GetCurrentPool()
	{
		return _localPools.GetValue();
	}
} // namespace RN
