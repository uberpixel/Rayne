//
//  RNWorkGroup.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorkGroup.h"
#include "../Data/RNRRef.h"

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

		auto rref = MakeRRef(std::move(function));

		queue->Perform([this, rref]() mutable {

			Function func(rref.Move());
			func();

			Leave();
		});
	}

	void WorkGroup::Enter()
	{
		if((_open ++) == 0)
			Retain();
	}
	void WorkGroup::Leave()
	{
		if(_open.fetch_sub(1, std::memory_order_release) == 1)
		{
			std::atomic_thread_fence(std::memory_order_acquire);

			std::unique_lock<std::mutex> lock(_lock);
			_signal.notify_all();

			for(auto &pair : _waiters)
			{
				WorkQueue *queue = std::get<0>(pair);
				queue->Perform(std::move(std::get<1>(pair)));
				queue->Release();
			}

			_waiters.clear();

			Release();
		}
	}

	void WorkGroup::Wait()
	{
		std::unique_lock<std::mutex> lock(_lock);
		_signal.wait(lock, [&]() -> bool { return (_open.load() == 0); });
	}
	bool WorkGroup::WaitUntil(const Clock::time_point &timeout)
	{
		std::unique_lock<std::mutex> lock(_lock);
		return _signal.wait_until(lock, timeout, [&]() -> bool { return (_open.load() == 0); });
	}
	void WorkGroup::Notify(WorkQueue *queue, Function &&function)
	{
		std::lock_guard<std::mutex> lock(_lock);
		_waiters.push_back(std::make_pair(queue->Retain(), std::move(function)));
	}
}
