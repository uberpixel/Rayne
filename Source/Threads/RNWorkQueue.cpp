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
		_barrier(false)
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

		ReCalculateWidth(); // Start with at least one running thread

		if(_flags & kRNWorkQueueFlagMainThread) // The main queue still needs a buffer
		{
			WorkThread *fake = new WorkThread();
			fake->localOpen = 0;

			_threads.push_back(fake);
		}
	}

	WorkQueue::~WorkQueue()
	{
		/*for(Thread *thread : _threads)
			thread->Cancel();

		_workSignal.notify_all();

		for(Thread *thread : _threads)
		{
			// Wait for the thread to exit
			thread->WaitForExit();
		}*/
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

		_open.fetch_add(1, std::memory_order_release);
		ReCalculateWidth();

		// Submit the work item to the queue
		LockGuard<SpinLock> lock(_threadLock);

		WorkThread *least = nullptr;
		size_t leastCount = 0xfffffffff;

		for(WorkThread *thread : _threads)
		{
			size_t open = thread->localOpen.load(std::memory_order_acquire);

			if(open < leastCount)
			{
				least = thread;
				leastCount = open;
			}
		}

		RN_ASSERT(least, "Broken WorkQueue. All that is left now is hope and tears");

		least->localBuffer.Push(source);
		least->localOpen.fetch_add(1, std::memory_order_release);

		lock.Unlock();


		// Assure wake up
		if(_sleeping > 0 && _suspended == 0)
		{
			std::unique_lock<std::mutex> lock(_workLock);
			_workSignal.notify_one();
		}

		return source;
	}

	bool WorkQueue::PerformWorkMainThread()
	{
		return PerformWork(_threads.front());
	}

	bool WorkQueue::PerformWork(WorkThread *thread)
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
		WorkSource *source;
		bool result = ConditionalSpin(thread->localBuffer.Pop(source));
		if(!result)
			return false;

		bool isBarrier;
		if((isBarrier = source->TestFlag(WorkSource::Flags::Barrier)))
			_barrier.store(std::memory_order_release);

		_running.fetch_add(1, std::memory_order_relaxed);
		_open.fetch_sub(1, std::memory_order_relaxed);
		thread->localOpen.fetch_sub(1, std::memory_order_relaxed);


		// Barrier path
		if(isBarrier)
		{
			// If the work is a barrier, wait until all other work loads clear up
			if(!ConditionalSpin(_running.load(std::memory_order_acquire) > 1))
			{
				std::unique_lock<std::mutex> lock(_barrierLock);
				_barrierSignal.wait(lock, [&]() -> bool {
					return (_running.load() == 1);
				});
			}

			// Call out and perform the work
			std::atomic_thread_fence(std::memory_order_acquire);
			source->Callout();
			std::atomic_thread_fence(std::memory_order_release);

			// Signal that the barrier is done
			std::lock_guard<std::mutex> lock(_barrierLock);
			_barrier.store(false, std::memory_order_relaxed);
			_barrierSignal.notify_all();
		}
		else
		{
			// Call out and perform the work
			std::atomic_thread_fence(std::memory_order_acquire);
			source->Callout();
			std::atomic_thread_fence(std::memory_order_release);

			// Clean up
			if(_barrier.load(std::memory_order_acquire))
			{
				// A barrier is waiting to execute, signal it to be ready if needed
				std::lock_guard<std::mutex> lock(_barrierLock);
				if((--_running) == 1)
					_barrierSignal.notify_all();
			}
			else
			{
				_running.fetch_sub(1, std::memory_order_relaxed);
			}
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

	void WorkQueue::ThreadEntry(WorkThread *thread)
	{
		Thread *actual;

		do {
			actual = thread->thread;
		} while(!actual);

		while(!actual->IsCancelled())
		{
			if(!PerformWork(thread))
			{
				if(!ConditionalSpin(_open.load() > 0))
				{
					std::unique_lock<std::mutex> lock(_workLock);

					if(_open.load() == 0)
					{
						_sleeping.fetch_add(1, std::memory_order_relaxed);
						_workSignal.wait(lock, [&]() -> bool { return (_open.load(std::memory_order_acquire) > 0 || actual->IsCancelled()); });
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
				WorkThread *workThread = new WorkThread();
				workThread->localOpen.store(0, std::memory_order_release);
				workThread->thread = nullptr;

				Thread *thread = new Thread([this, workThread]{ ThreadEntry(workThread); });
				workThread->thread.store(thread, std::memory_order_release);

				_threads.push_back(workThread);
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
