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
		Perform(queue, std::move(function), 0);
	}
	void WorkGroup::Perform(WorkQueue *queue, Function &&function, size_t concurrencyHint)
	{
		Enter();

		try
		{
			queue->Perform([this, f = std::move(function)]() mutable {
				Function func(std::move(f));
				try
				{
					func();
				}
				catch(...)
				{
					Leave();
					throw;
				}

				Leave();
			},
						   concurrencyHint);
		}
		catch(...)
		{
			Leave();
			throw;
		}
	}

	void WorkGroup::Enter()
	{
		LockGuard<Lockable> lock(_lock);
		if(_open++ == 0)
			Retain();
	}
	void WorkGroup::Leave()
	{
		UniqueLock<Lockable> lock(_lock);
		if(--_open == 0)
		{
			decltype(_waiters) waiters;
			std::exception_ptr exception;
			waiters.swap(_waiters);
			try
			{
				_signal.NotifyAll();
			}
			catch(...)
			{
				exception = std::current_exception();
			}
			lock.Unlock();

			// A rejected notification must not strand the remaining captures or
			// references. Dispatch and destroy captures outside the group lock.
			for(auto &pair : waiters)
			{
				WorkQueue *queue = std::get<0>(pair);
				try
				{
					queue->Perform(std::move(std::get<1>(pair)));
				}
				catch(...)
				{
					if(!exception) exception = std::current_exception();
				}
				queue->Release();
			}
			waiters.clear();
			Release();
			if(exception) std::rethrow_exception(exception);
		}
	}

	void WorkGroup::Wait()
	{
		UniqueLock<Lockable> lock(_lock);
		_signal.Wait(lock, [&]() -> bool { return (_open == 0); });
	}
	bool WorkGroup::WaitUntil(const Clock::time_point &timeout)
	{
		UniqueLock<Lockable> lock(_lock);
		return _signal.WaitUntil(lock, timeout, [&]() -> bool { return (_open == 0); });
	}
	void WorkGroup::Notify(WorkQueue *queue, Function &&function)
	{
		{
			LockGuard<Lockable> lock(_lock);
			if(_open != 0)
			{
				_waiters.emplace_back(queue, std::move(function));
				queue->Retain();
				return;
			}
		}

		queue->Perform(std::move(function));
	}
} // namespace RN
