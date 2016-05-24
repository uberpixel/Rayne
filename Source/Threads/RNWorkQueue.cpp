//
//  RNWorkQueue.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorkQueue.h"
#include "RNThreadLocalStorage.h"
#include "../Objects/RNAutoreleasePool.h"
#include <boost/lockfree/queue.hpp>

#if RN_PLATFORM_INTEL
#if RN_PLATFORM_WINDOWS
#define RNHardwarePause() YieldProcessor()
#else
#define RNHardwarePause() __asm__ volatile("pause")
#endif
#endif
#if RN_PLATFORM_ARM
#define RNHardwarePause() __asm__ volatile("yield")
#endif

#define RNConditionalSpin(e, count, result) \
	do { \
		result = false; \
		for(size_t i = 0; i < count; i ++) \
		{ \
			if((e)) \
			{ \
				result = true; \
				break; \
			} \
			RNHardwarePause(); \
		} \
	} while(0)


#define ConditionalSpin(e, result) RNConditionalSpin(e, 10535U, result)
#define ConditionalSpinLow(e, result) RNConditionalSpin(e, 512U, result)

namespace RN
{
	RNDefineMeta(WorkQueue, Object)

	struct WorkQueueInternals
	{
		WorkQueueInternals() :
			workQueue(4096)
		{}

		std::vector<WorkSource *> overcommitQueue;
		std::atomic<bool> isOverCommitted;
		Lockable overcommitLock;

		Condition workSignal;
		Lockable workLock;

		boost::lockfree::queue<WorkSource *, boost::parameter::void_, boost::parameter::void_, boost::parameter::void_> workQueue;
	};

	// Private queue flags
	#define kRNWorkQueueFlagMainThread (1 << 18)

	static WorkQueue *__WorkQueues[4];
	static ThreadLocalStorage<WorkQueue *> __LocalWorkQueues;

	void WorkQueue::InitializeQueues()
	{
		__WorkQueues[0] = new WorkQueue(Priority::High, Flags::Concurrent, RNCSTR("net.uberpixel.rayne.queue.high"));
		__WorkQueues[1] = new WorkQueue(Priority::Default, Flags::Concurrent, RNCSTR("net.uberpixel.rayne.queue.default"));
		__WorkQueues[2] = new WorkQueue(Priority::Background, Flags::Concurrent, RNCSTR("net.uberpixel.rayne.queue.background"));
		__WorkQueues[3] = new WorkQueue(Priority::Default, kRNWorkQueueFlagMainThread, RNCSTR("net.uberpixel.rayne.queue.main"));
	}

	void WorkQueue::TearDownQueues()
	{
		for(size_t i = 0; i < 4; i ++)
		{
			__WorkQueues[i]->Release();
			__WorkQueues[i] = nullptr;
		}
	}


	WorkQueue::WorkQueue(Priority priority, Flags flags, const String *identifier) :
		_flags(flags),
		_concurrency(std::thread::hardware_concurrency()),
		_width(0),
		_realWidth(0),
		_open(0),
		_suspended(0),
		_barrier(false),
		_identifier(identifier->Copy()),
		_threadCount(0)
	{
		size_t multiplier;
		size_t maxThreads;

		switch(priority)
		{
			case Priority::High:
				multiplier = 8;
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
		_threadLock.Lock();
		std::vector<Thread *> threads = _threads;
		_threadLock.Unlock();


		// Cancel all threads, wake them up and then wait for their exit
		for(Thread *thread : threads)
			thread->Cancel();

		_internals->workSignal.NotifyAll();

		for(Thread *thread : threads)
			thread->WaitForExit();

		_identifier->Release();
	}


	WorkQueue *WorkQueue::GetMainQueue()
	{
		return __WorkQueues[3];
	}
	WorkQueue *WorkQueue::GetGlobalQueue(Priority priority)
	{
		return __WorkQueues[static_cast<uint32>(priority)];
	}

	WorkQueue *WorkQueue::GetCurrentWorkQueue()
	{
		Thread *thread = Thread::GetCurrentThread();
		if(thread == Thread::GetMainThread())
			return GetMainQueue();

		return __LocalWorkQueues.GetValue();
	}

	bool WorkQueue::CanYield() const
	{
		return (((_flags & kRNWorkQueueFlagMainThread) == 0) && this == __LocalWorkQueues.GetValue());
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

		UniqueLock<Lockable> lock(_syncLock);
		if(source->IsComplete())
		{
			source->Relinquish();
			return;
		}

		_syncSignal.Wait(lock, [&]{ return (source->IsComplete()); });
		source->Relinquish();
	}
	void WorkQueue::PerformSynchronousBarrier(Function &&function)
	{
		WorkSource *source = PerformWithFlags(std::move(function), WorkSource::Flags::Synchronous | WorkSource::Flags::Barrier);

		UniqueLock<Lockable> lock(_syncLock);
		if(source->IsComplete())
		{
			source->Relinquish();
			return;
		}

		_syncSignal.Wait(lock, [&]{ return (source->IsComplete()); });
		source->Relinquish();
	}

	WorkSource *WorkQueue::PerformWithFlags(Function &&function, WorkSource::Flags flags)
	{
		WorkSource *source = WorkSource::DequeueWorkSource(std::move(function), flags);

		_internals->workQueue.push(source);
		_open.fetch_add(1, std::memory_order_relaxed);

		if(flags & WorkSource::Flags::Barrier)
		{
			// Push _concurrency BarrierBlock work sources to keep other threads
			// from running over the barrier. This works because threads encountering a BarrierBlock
			// will consume the block and wait until the barrier completes.
			// Also we only guarantee that barriers work with single producer submission

			for(size_t i = 0; i < _concurrency; i ++)
			{
				WorkSource *source = WorkSource::DequeueWorkSource([]{}, WorkSource::Flags::Barrier | WorkSource::Flags::BarrierBlock);
				_internals->workQueue.push(source);
				_open.fetch_add(1, std::memory_order_relaxed);
			}
		}

		ReCalculateWidth();

		// Assure wake up
		if(_sleeping.load(std::memory_order_acquire) && _suspended.load(std::memory_order_acquire) == 0)
		{
			UniqueLock<Lockable> lock(_internals->workLock);
			_internals->workSignal.NotifyOne();
		}

		return source;
	}

	bool WorkQueue::PerformWork()
	{
		bool result;
		ConditionalSpin(_suspended.load(std::memory_order_acquire) == 0, result);

		if(!result)
		{
			UniqueLock<Lockable> lock(_internals->workLock);
			_internals->workSignal.Wait(lock, [&]() -> bool { return (_suspended.load(std::memory_order_acquire) == 0); });
		}

		ConditionalSpin(_barrier.load(std::memory_order_acquire) == false, result);
		if(!result)
		{
			// There currently is a barrier executing, so wait until that one is completed
			UniqueLock<Lockable> lock(_barrierLock);
			_barrierSignal.Wait(lock, [&]() -> bool { return (_barrier.load(std::memory_order_acquire) == false); });
		}

		// Grab work from the work pool
		WorkSource *source;
		ConditionalSpinLow(_internals->workQueue.pop(source), result);

		if(!result)
			return false;

		bool isBarrier;
		if((isBarrier = source->TestFlag(WorkSource::Flags::Barrier)))
			_barrier.store(true, std::memory_order_release);

		_running.fetch_add(1, std::memory_order_relaxed);
		_open.fetch_sub(1, std::memory_order_relaxed);


		// Barrier path
		if(isBarrier)
		{
			if(source->TestFlag(WorkSource::Flags::BarrierBlock))
			{
				// Give the barrier thread a bit of time to set the barrier flag
				// This avoids racing without introducing a lock
				ConditionalSpinLow(_barrier.load(std::memory_order_acquire) == true, result);

				// Wait for rendezvous with the barrier signal
				UniqueLock<Lockable> lock(_barrierLock);
				_barrierSignal.Wait(lock, [&]() -> bool {
					return (_barrier.load(std::memory_order_acquire) == false);
				});

				source->Relinquish();
				return true;
			}

			// If the work is a barrier, wait until all other work loads clear up
			ConditionalSpin(_running.load(std::memory_order_acquire) > 1, result);

			if(!result)
			{
				UniqueLock<Lockable> lock(_barrierLock);
				_barrierSignal.Wait(lock, [&]() -> bool {
					return (_running.load() == 1);
				});
			}

			// Call out and perform the work
			std::atomic_thread_fence(std::memory_order_acquire);
			source->Callout();
			std::atomic_thread_fence(std::memory_order_release);

			// Signal that the barrier is done
			LockGuard<Lockable> lock(_barrierLock);
			_barrier.store(false, std::memory_order_relaxed);
			_barrierSignal.NotifyAll();
		}
		else // Non barrier path
		{
			// Call out and perform the work
			std::atomic_thread_fence(std::memory_order_acquire);
			source->Callout();
			std::atomic_thread_fence(std::memory_order_release);

			// Clean up
			if(_barrier.load(std::memory_order_acquire))
			{
				// A barrier is waiting to execute, signal it to be ready if needed
				LockGuard<Lockable> lock(_barrierLock);
				if((--_running) == 1)
					_barrierSignal.NotifyAll();
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

			LockGuard<Lockable> lock(_syncLock);
			source->Complete();
			_syncSignal.NotifyAll();

			return true;
		}

		source->Relinquish();
		return true;
	}

	bool WorkQueue::PerformWorkWithTimeout(uint32 timeout)
	{
		Thread *thread = Thread::GetCurrentThread();
		AutoreleasePool pool;

		__LocalWorkQueues.SetValue(this);

		while(!thread->IsCancelled())
		{
			if(!PerformWork())
			{
				bool result;
				ConditionalSpin(_open.load(std::memory_order_acquire) > 0, result);

				if(!result)
				{
					UniqueLock<Lockable> lock(_internals->workLock);

					if(_open.load() == 0)
					{
						pool.Drain();

						_sleeping.fetch_add(1, std::memory_order_relaxed);
						bool result = _internals->workSignal.WaitFor(lock, std::chrono::milliseconds(timeout), [&]() -> bool { return (_open.load(std::memory_order_acquire) > 0 || thread->IsCancelled()); });
						_sleeping.fetch_sub(1, std::memory_order_relaxed);

						// Resign the thread if the timeout was reached
						if(RN_EXPECT_FALSE(!result))
							return false;
					}
				}
			}
		}

		return true;
	}

	void WorkQueue::__Yield()
	{
		PerformWorkWithTimeout(1);
	}

	void WorkQueue::ThreadEntry()
	{
		if(!PerformWorkWithTimeout(500))
		{
			Thread *thread = Thread::GetCurrentThread();

			_threadLock.Lock();

			_threads.erase(std::find(_threads.begin(), _threads.end(), thread));
			_width--;

			_threadLock.Unlock();
		}
	}

	void WorkQueue::ReCalculateWidth()
	{
		if(_flags & kRNWorkQueueFlagMainThread)
			return;

		size_t width;

		if(_flags & WorkQueue::Flags::Concurrent)
		{
			size_t open = std::min(_threshold, _open.load(std::memory_order_acquire));
			width = static_cast<size_t>(roundf(_concurrency * (open / static_cast<float>(_threshold))));

			if(_open.load(std::memory_order_acquire) > 0)
				width = std::max(width, static_cast<size_t>(1));
		}
		else
		{
			width = (_open.load(std::memory_order_acquire) > 0) ? 1 : 0;
		}

		_threadLock.Lock();
		_realWidth = width;

		if(width > _width)
		{
			for(size_t i = 0; i < width - _width; i ++)
			{
				Thread *thread = new Thread([this]{ ThreadEntry(); }, false);
				thread->SetName(_identifier->StringByAppendingString(RNSTR("." << _threadCount ++)));
				thread->Start();

				_threads.push_back(thread);
				thread->Release();
			}

			_width = width;
		}
		_threadLock.Unlock();
	}

	void WorkQueue::Suspend()
	{
		_suspended.fetch_add(1, std::memory_order_relaxed);
	}
	void WorkQueue::Resume()
	{
		if(_suspended.fetch_sub(1, std::memory_order_acquire) == 1)
			_internals->workSignal.NotifyAll();
	}
}
