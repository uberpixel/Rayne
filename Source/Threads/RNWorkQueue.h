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
			RN_ASSERT(CanYield(), "Only yieldable queues can be yielded");

			while(!predicate())
				__Yield();
		}

		template<class T>
		void YieldWithFuture(T &future)
		{
			if(RN_EXPECT_FALSE(!CanYield()))
			{
				future.wait();
				return;
			}

			YieldWithPredicate([&]() -> bool {
				return (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
			});
		}

		template<class T>
		void YieldWithCondition(Condition &condition, T &lock)
		{
			if(RN_EXPECT_FALSE(!CanYield()))
			{
				condition.Wait(lock);
				return;
			}

			YieldWithPredicate([&]() -> bool {
				return (condition.WaitFor(lock, std::chrono::seconds(0)));
			});
		}

		template<class T, class Predicate>
		void YieldWithCondition(Condition &condition, T &lock, Predicate &&predicate)
		{
			if(RN_EXPECT_FALSE(!CanYield()))
			{
				condition.Wait(lock, std::move(predicate));
				return;
			}

			YieldWithPredicate([&]() -> bool {
				return condition.WaitFor(lock, std::chrono::seconds(0), std::move(predicate));
			});
		}

		RNAPI void Suspend();
		RNAPI void Resume();

		RNAPI bool CanYield() const;

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

		Condition _barrierSignal;
		Lockable _barrierLock;

		Condition _syncSignal;
		Lockable _syncLock;

		Lockable _threadLock;
		std::vector<Thread *> _threads;

		PIMPL<WorkQueueInternals> _internals;

		__RNDeclareMetaInternal(WorkQueue)
	};

	RNObjectClass(WorkQueue)
}

#endif /* __RAYNE_WORKQUEUE_H__ */
