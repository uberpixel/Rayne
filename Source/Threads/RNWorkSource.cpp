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
		WorkSourcePool()
		{
			// Pre-Warm the local work group
			for(size_t i = 0; i < 2048; i ++)
			{
				WorkSource *source = new WorkSource(this);
				buffer.Push(source);
			}
		}

		SpinLock readLock;
		SpinLock writeLock;

		AtomicRingBuffer<WorkSource *, 16192> buffer;
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

	WorkSource::WorkSource(WorkSourcePool *pool) :
		_pool(pool)
	{}

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
			lock.Unlock();

			RNDebug("Exhausted local work pool");
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
		_completed = false;
	}

	void WorkSource::Relinquish()
	{
		LockGuard<SpinLock> lock(_pool->writeLock);

		if(!_pool->buffer.Push(this))
		{
			lock.Unlock();
			delete this; // The local pool reached its limit
		}
	}
}
