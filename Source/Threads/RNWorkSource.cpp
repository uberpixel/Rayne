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

namespace RN
{
	struct WorkSourcePool
	{
		SpinLock readLock;
		SpinLock writeLock;

		AtomicRingBuffer<WorkSource *, 128> buffer;
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
		_completed(false)
	{}

	WorkSource *WorkSource::DequeueWorkSource(Function &&function, Flags flags)
	{
		WorkSourcePool *pool = __GetLocalPool();
		WorkSource *source;

		LockGuard<SpinLock> lock(pool->readLock);

		if(!pool->buffer.Pop(source))
		{
			source = new WorkSource(std::move(function), flags, pool);
			return source;
		}

		lock.Unlock();

		source->Refurbish(std::move(function), flags);
		return source;
	}

	void WorkSource::Refurbish(Function &&function, Flags flags)
	{
		_function = std::move(function);
		_flags = flags;
		_completed  = false;
	}

	void WorkSource::Relinquish()
	{
		LockGuard<SpinLock> lock(_pool->writeLock);

		if(!_pool->buffer.Push(this))
			delete this; // The local pool reached its limit
	}
}
