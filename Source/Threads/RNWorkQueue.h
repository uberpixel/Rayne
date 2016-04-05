//
//  RNWorkQueue.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WORKQUEUE_H__
#define __RAYNE_WORKQUEUE_H__

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Data/RNRingBuffer.h"
#include "RNWorkSource.h"
#include "RNThread.h"

namespace RN
{
	class Kernel;
	struct WorkQueueInternals;

	class WorkQueue : public Object
	{
	public:
		friend class Kernel;

		RN_OPTIONS(Flags, uint32,
				   Serial = 0,
				   Concurrent = (1 << 0));

		enum class Priority : uint32
		{
			High = 0,
			Default = 1,
			Background = 2
		};

		RNAPI WorkQueue(Priority priority, Flags flags, const String *identifier);
		RNAPI ~WorkQueue();

		RNAPI static WorkQueue *GetMainQueue();
		RNAPI static WorkQueue *GetGlobalQueue(Priority priority);
		RNAPI static WorkQueue *GetCurrentWorkQueue();

		RNAPI void Perform(Function &&function);
		RNAPI void PerformBarrier(Function &&function);
		RNAPI void PerformSynchronous(Function &&function);
		RNAPI void PerformSynchronousBarrier(Function &&function);

		template<class F>
		std::future<typename std::result_of<F()>::type> PerformWithFuture(F &&f)
		{
			typedef typename std::result_of<F()>::type resultType;

			std::promise<resultType> promise;
			std::future<resultType> result(promise.get_future());

			Perform([promise = std::move(promise), f = std::move(f)]() mutable {

				try
				{
					promise.set_value(f());
				}
				catch(...)
				{
					promise.set_exception(std::current_exception());
				}

			});

			return result;
		}

		template<class F>
		std::future<typename std::result_of<F()>::type> PerformBarrierWithFuture(F &&f)
		{
			typedef typename std::result_of<F()>::type resultType;

			std::promise<resultType> promise;
			std::future<resultType> result(promise.get_future());

			Perform([promise = std::move(promise), f = std::move(f)]() mutable {

				try
				{
					promise.set_value(f());
				}
				catch(...)
				{
					promise.set_exception(std::current_exception());
				}

			});

			return result;
		}

		template<class Predicate>
		void YieldWithPredicate(Predicate &&predicate)
		{
			while(!predicate())
				__Yield();
		}

		template<class T>
		void YieldWithFuture(T &future)
		{
			YieldWithPredicate([&]() -> bool {
				return (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
			});
		}

		template<class T>
		void YieldWithCondition(std::condition_variable &condition, T &lock)
		{
			YieldWithPredicate([&]() -> bool {
				return (condition.wait_for(lock, std::chrono::seconds(0)) == std::cv_status::no_timeout);
			});
		}

		template<class T, class Predicate>
		void YieldWithCondition(std::condition_variable &condition, T &lock, Predicate &&predicate)
		{
			YieldWithPredicate([&]() -> bool {
				return condition.wait_for(lock, std::chrono::seconds(0), std::move(predicate));
			});
		}

		RNAPI void Suspend();
		RNAPI void Resume();

	private:
		static void InitializeQueues();
		static void TearDownQueues();

		WorkSource *PerformWithFlags(Function &&function, WorkSource::Flags flags);

		RNAPI void __Yield();

		void ThreadEntry();
		bool PerformWorkWithTimeout(uint32 timeout);
		bool PerformWork();

		void ReCalculateWidth();

		String *_identifier;
		Flags _flags;

		size_t _concurrency;
		size_t _threshold;

		size_t _width;
		size_t _realWidth;
		size_t _threadCount;

		std::atomic<size_t> _open;
		std::atomic<size_t> _running;
		std::atomic<size_t> _sleeping;
		std::atomic<size_t> _suspended;
		std::atomic<bool> _barrier;

		std::condition_variable _barrierSignal;
		std::mutex _barrierLock;

		std::condition_variable _syncSignal;
		std::mutex _syncLock;

		SpinLock _threadLock;
		std::vector<Thread *> _threads;

		PIMPL<WorkQueueInternals> _internals;

		__RNDeclareMetaInternal(WorkQueue)
	};

	RNObjectClass(WorkQueue)
}

#endif /* __RAYNE_WORKQUEUE_H__ */
