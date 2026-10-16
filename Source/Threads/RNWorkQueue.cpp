//
//  RNWorkQueue.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorkQueue.h"
#include "../Objects/RNAutoreleasePool.h"
#include "RNThreadLocalStorage.h"
#include <concurrentqueue.h>

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
	do {                                    \
		result = false;                     \
		for(size_t i = 0; i < count; i++)   \
		{                                   \
			if((e))                         \
			{                               \
				result = true;              \
				break;                      \
			}                               \
			RNHardwarePause();              \
		}                                   \
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

		Condition workSignal;
		Lockable workLock;

		moodycamel::ConcurrentQueue<WorkSource *> workQueue;
	};

// Private queue flags
#define kRNWorkQueueFlagMainThread (1 << 18)

	static WorkQueue *__WorkQueues[4];
	static ThreadLocalStorage<WorkQueue *> __LocalWorkQueues;

	void WorkQueue::InitializeQueues()
	{
		__WorkQueues[0] = new WorkQueue(Priority::High, Flags::Concurrent, RNCSTR("RN::High"));
		__WorkQueues[1] = new WorkQueue(Priority::Default, Flags::Concurrent, RNCSTR("RN::Default"));
		__WorkQueues[2] = new WorkQueue(Priority::Background, Flags::Concurrent, RNCSTR("RN::Backgrnd")); //Gets truncated on android if too long, so shorten a little
		__WorkQueues[3] = new WorkQueue(Priority::Default, kRNWorkQueueFlagMainThread, RNCSTR("RN::Main"));
	}

	void WorkQueue::TearDownQueues()
	{
		for(size_t i = 0; i < 4; i++)
		{
			__WorkQueues[i]->Release();
			__WorkQueues[i] = nullptr;
		}
	}


	WorkQueue::WorkQueue(Priority priority, Flags flags, const String *identifier) :
		_identifier(identifier->Copy()),
		_flags(flags),
		_priority(priority),
		_concurrency(std::thread::hardware_concurrency()),
		_width(0),
		_realWidth(0),
		_threadCount(0),
		_open(0),
		_running(0),
		_sleeping(0),
		_suspended(0),
		_barrier(false),
		_pendingBarrier(nullptr)
	{
		size_t multiplier = 16;
		size_t maxThreads = 32;

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
		// Worker threads can remove themselves from _threads while shutting down; keep the objects alive until after WaitForExit().
		for(Thread *thread : threads)
			thread->Retain();
		_threadLock.Unlock();

		// Cancel all threads, wake them up and then wait for their exit
		for(Thread *thread : threads)
			thread->Cancel();

		// Wake every internal wait path so canceled workers can reach their exit check.
		_suspended.store(0, std::memory_order_release);
		{
			LockGuard<Lockable> lock(_barrierLock);
			_barrier.store(false, std::memory_order_release);
			_barrierSignal.NotifyAll();
		}
		{
			LockGuard<Lockable> lock(_syncLock);
			_syncSignal.NotifyAll();
		}

		{
			LockGuard<Lockable> lock(_internals->workLock);
			_internals->workSignal.NotifyAll();
		}

		for(Thread *thread : threads)
		{
			thread->WaitForExit();
			thread->Release();
		}

		if(_pendingBarrier)
		{
			if(_pendingBarrier->TestFlag(WorkSource::Flags::Synchronous))
			{
				// The synchronous caller owns the source until completion.
				_pendingBarrier->_function = Function();
				LockGuard<Lockable> lock(_syncLock);
				_pendingBarrier->Complete();
				_syncSignal.NotifyAll();
			}
			else
				_pendingBarrier->Relinquish();
		}

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
		Perform(std::move(function), 0);
	}
	void WorkQueue::Perform(Function &&function, size_t concurrencyHint)
	{
		PerformWithFlags(std::move(function), 0, concurrencyHint);
	}
	void WorkQueue::PerformBarrier(Function &&function)
	{
		// Serial queues already execute work exclusively.
		PerformWithFlags(std::move(function), (_flags & Flags::Concurrent) ? WorkSource::Flags::Barrier : 0);
	}

	void WorkQueue::PerformSynchronous(Function &&function)
	{
		WorkSource *source = PerformWithFlags(std::move(function), WorkSource::Flags::Synchronous);

		{
			UniqueLock<Lockable> lock(_syncLock);
			_syncSignal.Wait(lock, [&] { return source->IsComplete(); });
		}
		std::exception_ptr exception = source->_exception;
		source->Relinquish();
		if(exception) std::rethrow_exception(exception);
	}
	void WorkQueue::PerformSynchronousBarrier(Function &&function)
	{
		WorkSource *source = PerformWithFlags(std::move(function), WorkSource::Flags::Synchronous | ((_flags & Flags::Concurrent) ? WorkSource::Flags::Barrier : 0));
		{
			UniqueLock<Lockable> lock(_syncLock);
			_syncSignal.Wait(lock, [&] { return source->IsComplete(); });
		}
		std::exception_ptr exception = source->_exception;
		source->Relinquish();
		if(exception) std::rethrow_exception(exception);
	}

	WorkSource *WorkQueue::PerformWithFlags(Function &&function, WorkSource::Flags flags, size_t concurrencyHint)
	{
		std::shared_ptr<std::atomic<bool>> completed;
		if(flags & WorkSource::Flags::Barrier)
		{
			completed = std::make_shared<std::atomic<bool>>(false);
			function = [function = std::move(function), completed]() mutable {
				try
				{
					function();
				}
				catch(...)
				{
					completed->store(true);
					throw;
				}
				completed->store(true);
			};
		}
		WorkSource *source = WorkSource::DequeueWorkSource(std::move(function), flags);

		std::vector<WorkSource *> batch;
		const size_t count = (flags & WorkSource::Flags::Barrier) ? _concurrency + 1 : 1;
		bool counted = false;
		UniqueLock<Lockable> wakeLock(_internals->workLock, LockPolicy::DeferLock);
		try
		{
			if(flags & WorkSource::Flags::Barrier)
			{
				batch.reserve(count);
				batch.push_back(source);
				// Publish the barrier and all admission markers together. Ordering
				// between barriers and other work still requires a single producer.
				for(size_t i = 0; i < _concurrency; i++)
				{
					batch.push_back(WorkSource::DequeueWorkSource([this, completed] {
						LockGuard<Lockable> lock(_barrierLock);
						if(!completed->load())
							_barrier.store(true);
					},
																  WorkSource::Flags::BarrierBlock));
				}
			}

			// Prepare workers before publication so a failed start cannot strand
			// accepted work. Count first so workers cannot retire or decrement early.
			_open.fetch_add(count, std::memory_order_relaxed);
			counted = true;
			ReCalculateWidth(concurrencyHint);

			// Keep sleepers locked out until the work is published or its count
			// has been rolled back.
			if(_sleeping.load(std::memory_order_acquire) && _suspended.load(std::memory_order_acquire) == 0)
			{
				wakeLock.Lock();
				_internals->workSignal.NotifyOne();
			}

			bool accepted = batch.empty() ? _internals->workQueue.enqueue(source) : _internals->workQueue.enqueue_bulk(batch.begin(), batch.size());
			if(!accepted) throw std::bad_alloc();
		}
		catch(...)
		{
			if(counted) _open.fetch_sub(count, std::memory_order_relaxed);
			if(wakeLock.OwnsLock()) wakeLock.Unlock();
			source->Relinquish();
			for(size_t i = 1; i < batch.size(); i++)
				batch[i]->Relinquish();
			throw;
		}

		return source;
	}

	void WorkQueue::FinishWork()
	{
		if(_running.fetch_sub(1) == 1 && _barrier.load())
		{
			LockGuard<Lockable> lock(_barrierLock);
			_barrierSignal.NotifyAll();
		}
	}

	bool WorkQueue::PerformWork(bool yielding)
	{
		Thread *thread = Thread::GetCurrentThread();
		bool result;
		if(yielding && _suspended.load()) return false;
		ConditionalSpin(_suspended.load(std::memory_order_acquire) == 0, result);
		if(!result)
		{
			if(yielding) return false;
			UniqueLock<Lockable> lock(_internals->workLock);
			_internals->workSignal.Wait(lock, [&] { return _suspended.load() == 0 || thread->IsCancelled(); });
		}
		if(thread->IsCancelled()) return false;

		WorkSource *source = nullptr;
		if(_barrier.load())
		{
			if(yielding) return false;
			UniqueLock<Lockable> lock(_barrierLock);
			_barrierSignal.Wait(lock, [&] {
				return !_barrier.load() || (_pendingBarrier && _running.load() == 0) || thread->IsCancelled();
			});
			if(thread->IsCancelled()) return false;
			if(_barrier.load())
			{
				source = _pendingBarrier;
				_pendingBarrier = nullptr;
				_running.fetch_add(1);
			}
		}

		if(!source)
		{
			// Reserve before dequeue so a barrier also counts paused consumers.
			_running.fetch_add(1);
			if(_barrier.load())
			{
				FinishWork();
				return false;
			}
			ConditionalSpinLow(_internals->workQueue.try_dequeue(source), result);
			if(!result)
			{
				FinishWork();
				return false;
			}
			_open.fetch_sub(1, std::memory_order_relaxed);

			if(source->TestFlag(WorkSource::Flags::BarrierBlock))
			{
				// Markers close admission without waiting inside a yielding callback.
				source->Callout();
				FinishWork();
				source->Relinquish();
				return true;
			}
			if(source->TestFlag(WorkSource::Flags::Barrier))
			{
				{
					LockGuard<Lockable> lock(_barrierLock);
					_pendingBarrier = source;
					_barrier.store(true);
				}
				FinishWork();
				return true;
			}
		}

		std::atomic_thread_fence(std::memory_order_acquire);
		source->Callout();
		std::atomic_thread_fence(std::memory_order_release);
		if(source->TestFlag(WorkSource::Flags::Barrier))
		{
			LockGuard<Lockable> lock(_barrierLock);
			_running.fetch_sub(1);
			_barrier.store(false);
			_barrierSignal.NotifyAll();
		}
		else
			FinishWork();

		if(source->TestFlag(WorkSource::Flags::Synchronous))
		{
			LockGuard<Lockable> lock(_syncLock);
			source->Complete();
			_syncSignal.NotifyAll();
		}
		else
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
					pool.Drain();
					UniqueLock<Lockable> lock(_internals->workLock);

					if(_open.load() == 0)
					{
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
		auto canPerformWork = [this] {
			return _open.load() > 0 && !_barrier.load() && !_suspended.load();
		};
		if(canPerformWork())
		{
			AutoreleasePool pool;
			if(PerformWork(true)) return;
		}

		Thread *thread = Thread::GetCurrentThread();
		UniqueLock<Lockable> lock(_internals->workLock);
		_sleeping.fetch_add(1, std::memory_order_relaxed);
		_internals->workSignal.WaitFor(lock, std::chrono::milliseconds(1), [&] { return canPerformWork() || thread->IsCancelled(); });
		_sleeping.fetch_sub(1, std::memory_order_relaxed);
	}

	void WorkQueue::ThreadEntry()
	{
		Thread::SetCurrentThreadPriority(_priority);
		while(!PerformWorkWithTimeout(500))
		{
			Thread *thread = Thread::GetCurrentThread();

			_threadLock.Lock();

			// Hand queued work back to this worker before committing its retirement.
			if(_open.load() > 0)
			{
				_threadLock.Unlock();
				continue;
			}

			_threads.erase(std::find(_threads.begin(), _threads.end(), thread));
			_width--;

			_threadLock.Unlock();
			break;
		}
	}

	void WorkQueue::ReCalculateWidth(size_t concurrencyHint)
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

			width = std::max(width, std::min(concurrencyHint, _concurrency));
		}
		else
		{
			width = (_open.load(std::memory_order_acquire) > 0) ? 1 : 0;
		}

		LockGuard<Lockable> lock(_threadLock);
		_realWidth = width;

		if(width > _width)
		{
			_threads.reserve(_threads.size() + width - _width);
			while(_width < width)
			{
				Thread *thread = new Thread([this] { ThreadEntry(); }, false);
				try
				{
					thread->SetName(_identifier->StringByAppendingString(RNSTR("." << _threadCount++)));
					thread->Start();
				}
				catch(...)
				{
					// No OS thread was created; release its reserved self reference too.
					thread->Release();
					thread->Release();
					throw;
				}

				_threads.push_back(thread);
				_width++;
				thread->Release();
			}
		}
	}

	void WorkQueue::Suspend()
	{
		_suspended.fetch_add(1, std::memory_order_relaxed);
	}
	void WorkQueue::Resume()
	{
		if(_suspended.fetch_sub(1, std::memory_order_acquire) == 1)
		{
			LockGuard<Lockable> lock(_internals->workLock);
			_internals->workSignal.NotifyAll();
		}
	}
} // namespace RN
