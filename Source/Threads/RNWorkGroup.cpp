//
//  RNWorkGroup.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorkGroup.h"

namespace RN
{
	RNDefineMeta(WorkGroup, Object)

	WorkGroup::WorkGroup() :
		_open(0)
	{}

	WorkGroup::~WorkGroup()
	{
		for(auto &pair : _waiters)
		{
			WorkQueue *queue = std::get<0>(pair);
			queue->Release();
		}
	}

	void WorkGroup::Perform(WorkQueue *queue, Function &&function)
	{
		Enter();

		queue->Perform([this, f = std::move(function)]() mutable {

			Function func(std::move(f));
			func();

			Leave();
		});
	}

	void WorkGroup::Enter()
	{
		if((_open.fetch_add(1, std::memory_order_acquire)) == 0)
			Retain();
	}
	void WorkGroup::Leave()
	{
		if(_open.fetch_sub(1, std::memory_order_release) == 1)
		{
			std::atomic_thread_fence(std::memory_order_acquire);

			{
				LockGuard<Lockable> lock(_lock);
				_signal.NotifyAll();

				for(auto &pair : _waiters)
				{
					WorkQueue *queue = std::get<0>(pair);
					queue->Perform(std::move(std::get<1>(pair)));
					queue->Release();
				}

				_waiters.clear();
			}

			Release();
		}
	}

	void WorkGroup::Wait()
	{
		UniqueLock<Lockable> lock(_lock);
		_signal.Wait(lock, [&]() -> bool { return (_open.load() == 0); });
	}
	bool WorkGroup::WaitUntil(const Clock::time_point &timeout)
	{
		UniqueLock<Lockable> lock(_lock);
		return _signal.WaitUntil(lock, timeout, [&]() -> bool { return (_open.load() == 0); });
	}
	void WorkGroup::Notify(WorkQueue *queue, Function &&function)
	{
		LockGuard<Lockable> lock(_lock);
		_waiters.push_back(std::make_pair(queue->Retain(), std::move(function)));
	}
}
