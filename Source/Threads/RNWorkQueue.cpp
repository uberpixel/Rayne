//
//  RNWorkQueue.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorkQueue.h"

#if RN_PLATFORM_INTEL
	#if RN_PLATFORM_WINDOWS
		#define RNHardwarePause() YieldProcessor()
	#else
		#define RNHardwarePause() __asm__("pause")
	#endif
#endif
#if RN_PLATFORM_ARM
	#define RNHardwarePause() __asm__("yield")
#endif

#define ConditionalSpin(e) \
	({ \
		bool result = false; \
		for(size_t i = 0; i < 65535U; i ++) \
		{ \
			if((e)) \
			{ \
				result = true; \
				break; \
			} \
			RNHardwarePause(); \
		} \
		result; \
	})

namespace RN
{
	RNDefineMeta(WorkQueue, Object)

	// Private queue flags
	#define kRNWorkQueueFlagMainThread (1 << 18)

	static WorkQueue *__WorkQueues[4];

	void WorkQueue::InitializeQueues()
	{
		__WorkQueues[0] = new WorkQueue(Priority::High, Flags::Concurrent);
		__WorkQueues[1] = new WorkQueue(Priority::Default, Flags::Concurrent);
		__WorkQueues[2] = new WorkQueue(Priority::Background, Flags::Concurrent);
		__WorkQueues[3] = new WorkQueue(Priority::Default, kRNWorkQueueFlagMainThread);
	}

	void WorkQueue::TearDownQueues()
	{
		for(size_t i = 0; i < 4; i ++)
		{
			__WorkQueues[i]->Release();
			__WorkQueues[i] = nullptr;
		}
	}


	WorkQueue::WorkQueue(Priority priority, Flags flags) :
		_flags(flags),
		_concurrency(std::thread::hardware_concurrency()),
		_width(0),
		_realWidth(0),
		_open(0),
		_sleeping(0),
		_suspended(0),
		_barrier(false),
		_isOverCommitted(false)
	{
		size_t multiplier;
		size_t maxThreads;

		switch(priority)
		{
			case Priority::High:
				multiplier = 12;
				maxThreads = 64;
				break;
			case Priority::Default:
				multiplier = 16;
				maxThreads = 32;
				break;
			case Priority::Background:
				multiplier = 32;
				maxThreads = 4;
				break;
		}

		_concurrency = std::max(static_cast<size_t>(2), _concurrency);
		_concurrency = std::min(_concurrency, maxThreads);
		_threshold = _concurrency * multiplier;
	}

	WorkQueue::~WorkQueue()
	{
		for(Thread *thread : _threads)
			thread->Cancel();

		_workSignal.notify_all();

		for(Thread *thread : _threads)
		{
			// Wait for the thread to exit
			thread->WaitForExit();
		}
	}


	WorkQueue *WorkQueue::GetMainQueue()
	{
		return __WorkQueues[3];
	}
	WorkQueue *WorkQueue::GetGlobalQueue(Priority priority)
	{
		return __WorkQueues[static_cast<uint32>(priority)];
	}


	void WorkQueue::Perform(Function &&function)
	{
		PerformWithFlags(std::move(function), 0);
	}
	void WorkQueue::PerformBarrier(Function &&function)
	{
		PerformWithFlags(std::move(function), WorkSource::Flags::Barrier);
	}

	void WorkQueue::PerformSynchronous(Function &&function)
	{
		WorkSource *source = PerformWithFlags(std::move(function), WorkSource::Flags::Synchronous);

		std::unique_lock<std::mutex> lock(_syncLock);
		if(source->IsComplete())
		{
			source->Relinquish();
			return;
		}

		_syncSignal.wait(lock, [&]{ return (source->IsComplete()); });
		source->Relinquish();
	}
	void WorkQueue::PerformSynchronousBarrier(Function &&function)
	{
		WorkSource *source = PerformWithFlags(std::move(function), WorkSource::Flags::Synchronous | WorkSource::Flags::Barrier);

		std::unique_lock<std::mutex> lock(_syncLock);
		if(source->IsComplete())
		{
			source->Relinquish();
			return;
		}

		_syncSignal.wait(lock, [&]{ return (source->IsComplete()); });
		source->Relinquish();
	}

	WorkSource *WorkQueue::PerformWithFlags(Function &&function, WorkSource::Flags flags)
	{
		WorkSource *source = WorkSource::DequeueWorkSource(std::move(function), flags);

		LockGuard<SpinLock> lock(_writeLock);

		if(!_buffer.Push(source))
		{
			// Put it in the over commit bin
			_overcommit.push_back(source);
			_open.fetch_add(1, std::memory_order_release);
			_isOverCommitted = true;

			return source;
		}

		// Assure wake up
		if(_sleeping > 0 && _suspended == 0)
		{
			std::unique_lock<std::mutex> lock(_workLock);
			_open.fetch_add(1, std::memory_order_release);
			_workSignal.notify_all();
		}
		else
		{
			_open.fetch_add(1, std::memory_order_release);
		}

		ReCalculateWidth();
		return source;
	}

	bool WorkQueue::PerformWork()
	{
		if(!ConditionalSpin(_suspended.load(std::memory_order_acquire) == 0))
		{
			std::unique_lock<std::mutex> lock(_workLock);
			_workSignal.wait(lock, [&]() -> bool { return (_suspended.load(std::memory_order_acquire) == 0); });
		}

		if(!ConditionalSpin(_barrier.load(std::memory_order_acquire) == false))
		{
			// There currently is a barrier executing, so wait until that one is completed
			std::unique_lock<std::mutex> lock(_barrierLock);
			_barrierSignal.wait(lock, [&]() -> bool { return (_barrier.load(std::memory_order_acquire) == false); });
		}

		// Grab work from the work pool
		_readLock.Lock();

		if(_barrier.load(std::memory_order_acquire)) // Make sure there really isn't a barrier in place now
		{
			_readLock.Unlock();
			return true;
		}

		WorkSource *source;
		bool result, isBarrier;

		if((result = _buffer.Pop(source)))
		{
			if((isBarrier = source->TestFlag(WorkSource::Flags::Barrier)))
				_barrier.store(std::memory_order_release);

			_running.fetch_add(1, std::memory_order_relaxed);
			_open.fetch_sub(1, std::memory_order_relaxed);
		}

		_readLock.Unlock();


		if(result)
		{
			if(isBarrier)
			{
				// If the work is a barrier, wait until all other work loads clear up
				if(!ConditionalSpin(_running.load(std::memory_order_acquire) > 1))
				{
					std::unique_lock<std::mutex> lock(_barrierLock);
					_barrierSignal.wait(lock, [&]() -> bool { return (_running.load() == 1); });
				}
			}

			// Call out and perform the work
			std::atomic_thread_fence(std::memory_order_acquire);
			source->Callout();
			std::atomic_thread_fence(std::memory_order_release);


			// If we are over committed, try to clear up that over commit
			if(_isOverCommitted.load(std::memory_order_acquire) && _writeLock.TryLock())
			{
				while(!_overcommit.empty())
				{
					WorkSource *temp = _overcommit.front();

					if(!_buffer.Push(temp))
						break;

					_overcommit.erase(_overcommit.begin());
				}

				if(_overcommit.empty())
					_isOverCommitted.store(false, std::memory_order_relaxed);

				_writeLock.Unlock();
			}

			// Clean up
			if(isBarrier)
			{
				std::lock_guard<std::mutex> lock(_barrierLock);
				_barrier.store(false, std::memory_order_relaxed);
				_barrierSignal.notify_all();
			}
			else if(_barrier.load(std::memory_order_acquire))
			{
				// A barrier is waiting to execute, signal it to be ready if needed
				std::lock_guard<std::mutex> lock(_barrierLock);
				if((-- _running) == 1)
					_barrierSignal.notify_all();
			}
			else
			{
				_running.fetch_sub(1, std::memory_order_relaxed);
			}

			if(source->TestFlag(WorkSource::Flags::Synchronous))
			{
				// Synchronous work sources aren't immediately relinquished but
				// the waiting thread is signalled that the work is completed
				// and it will relinquish the source eventually

				std::lock_guard<std::mutex> lock(_syncLock);
				source->Complete();
				_syncSignal.notify_all();
			}
			else
			{
				source->Relinquish();
			}

			return true;
		}

		return false;
	}

	void WorkQueue::ThreadEntry()
	{
		Thread *thread = Thread::GetCurrentThread();

		while(!thread->IsCancelled())
		{
			if(!PerformWork())
			{
				if(!ConditionalSpin(_open.load() > 0))
				{
					std::unique_lock<std::mutex> lock(_workLock);

					if(_open.load() == 0)
					{
						_sleeping.fetch_add(1, std::memory_order_relaxed);
						_workSignal.wait(lock, [&]() -> bool { return (_open.load() > 0 || thread->IsCancelled()); });
						_sleeping.fetch_sub(1, std::memory_order_relaxed);
					}
				}
			}
		}
	}

	/**
	 * Must be called with _writeLock being held
	 */
	void WorkQueue::ReCalculateWidth()
	{
		if(_flags & kRNWorkQueueFlagMainThread)
			return;

		size_t width = 1;

		if(_flags & WorkQueue::Flags::Concurrent)
		{
			size_t open = std::min(_threshold, _open.load(std::memory_order_acquire));
			width = static_cast<size_t>(roundf(_concurrency * (open / static_cast<float>(_threshold))));

			if(width == 0)
				width = 1;
		}

		if(_open.load(std::memory_order_acquire) == 0)
			width = 0;

		_realWidth = width;

		if(width > _width)
		{
			LockGuard<SpinLock> lock(_threadLock);

			for(size_t i = 0; i < width - _width; i ++)
			{
				Thread *thread = new Thread([this]{ ThreadEntry(); });
				_threads.push_back(thread);
			}

			_width = width;
		}
	}

	void WorkQueue::Suspend()
	{
		_suspended.fetch_add(1, std::memory_order_relaxed);
	}
	void WorkQueue::Resume()
	{
		if(_suspended.load(std::memory_order_acquire) == 1)
		{
			std::lock_guard<std::mutex> lock(_workLock);
			_suspended.fetch_sub(1, std::memory_order_release);
			_workSignal.notify_all();
		}
		else
		{
			_suspended.fetch_sub(1, std::memory_order_release);
		}
	}
}
