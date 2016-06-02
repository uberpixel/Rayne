//
//  RNWorkSource.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorkSource.h"
#include "RNThreadLocalStorage.h"
#include "../Data/RNRingBuffer.h"
#include "../Debug/RNLogger.h"

namespace RN
{
	struct WorkSourcePool
	{
		Lockable lock;
		WorkSource *head;
	};

	static ThreadLocalStorage<WorkSourcePool *> __LocalPools;

	WorkSourcePool *__GetLocalPool()
	{
		WorkSourcePool *pool = __LocalPools.GetValue();
		if(RN_EXPECT_FALSE(!pool))
		{
			pool = new WorkSourcePool();
			__LocalPools.SetValue(pool);
		}

		return pool;
	}

	WorkSource::WorkSource(Function &&function, Flags flags, WorkSourcePool *pool) :
		_function(std::move(function)),
		_flags(flags),
		_pool(pool),
		_completed(false),
		_next(nullptr)
	{}

	WorkSource *WorkSource::DequeueWorkSource(Function &&function, Flags flags)
	{
		WorkSourcePool *pool = __GetLocalPool();
		pool->lock.Lock();

		WorkSource *source = pool->head;
		if(!source)
		{
			pool->lock.Unlock();

			source = new WorkSource(std::move(function), flags, pool);
			return source;
		}

		pool->head = source->_next;
		pool->lock.Unlock();

		source->Refurbish(std::move(function), flags);
		return source;
	}

	void WorkSource::Refurbish(Function &&function, Flags flags)
	{
		_function = std::move(function);
		_flags = flags;
		_completed = false;
		_next = nullptr;
	}

	void WorkSource::Relinquish()
	{
		_pool->lock.Lock();
		_next = _pool->head;
		_pool->head = this;
		_pool->lock.Unlock();
	}
}
