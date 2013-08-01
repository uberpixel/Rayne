//
//  RNThreadPool.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_THREADPOOL_H__
#define __RAYNE_THREADPOOL_H__

#include "RNBase.h"
#include "RNThread.h"
#include "RNFunction.h"
#include "RNArray.h"
#include "RNContext.h"
#include "RNAutoreleasePool.h"
#include "RNRingbuffer.h"

namespace RN
{
	class ThreadPool;
	
	class ThreadCoordinator : public Singleton<ThreadCoordinator>
	{
	friend class Thread;
	public:
		RNAPI ThreadCoordinator();
		
		RNAPI int32 AvailableConcurrency();
		RNAPI int32 BaseConcurrency() const { return _baseConcurrency; }
		
	private:
		void ConsumeConcurrency();
		void RestoreConcurrency();
		
		int32 _baseConcurrency;
		std::atomic<int32> _consumedConcurrency;
	};
	
	class ThreadPool : public Singleton<ThreadPool>
	{
	class Task;
	public:
		class Batch
		{
		friend class ThreadPool;
		public:
			template<class F>
			void AddTask(F&& f)
			{
				Task temp;
				temp.function = std::move(f);
				temp.batch    = this;
				
				_tasks.push_back(std::move(temp));
			}
			
			template<class F>
			std::future<typename std::result_of<F()>::type> AddTaskWithFuture(F&& f)
			{
				typedef typename std::result_of<F()>::type resultType;
				
				std::packaged_task<resultType ()> task(std::move(f));
				std::future<resultType> result(task.get_future());
				
				Task temp;
				temp.function = std::move(task);
				temp.batch    = this;
				
				_tasks.push_back(std::move(temp));
				
				return result;
			}
			
			RNAPI void Commit();
			RNAPI void Wait();
			
			RNAPI void Retain();
			RNAPI void Release();
			
			void Reserve(size_t size)
			{
				_tasks.reserve(size);
			}
			
			size_t TaskCount() const { return _tasks.size(); }
			
		private:
			Batch(ThreadPool *pool) :
				_openTasks(0)
			{
				_pool = pool;
			}
			
			void TryFeedingBack();
			
			ThreadPool *_pool;
			
			std::vector<Task> _tasks;
			std::atomic<uint32> _openTasks;
			std::atomic<uint32> _listener;
			
			std::mutex _lock;
			std::condition_variable _waitCondition;
		};
		
		friend class Batch;
		
		ThreadPool(size_t maxJobs=0, size_t maxThreads=0);
		~ThreadPool() override;
		
		template<class F>
		void AddTask(F&& f)
		{
			Task temp;
			temp.function = std::move(f);
			temp.batch    = nullptr;
			
			FeedTaskFastPath(std::move(temp));
		}
		
		template<class F>
		std::future<typename std::result_of<F()>::type> AddTaskWithFuture(F&& f)
		{
			typedef typename std::result_of<F()>::type resultType;
			
			std::packaged_task<resultType ()> task(std::move(f));
			std::future<resultType> result(task.get_future());
			
			Task temp;
			temp.function = std::move(task);
			temp.batch    = nullptr;
			
			FeedTaskFastPath(std::move(temp));
			
			return result;
		}
		
		Batch *OpenBatch();
		
	private:		
		class Task
		{
		public:
			Task()
			{}
			
			Function function;
			Batch *batch;
		};
		
		Thread *CreateThread();
		
		void Consumer();
		void ReadTasks(std::vector<Task>& tasks);
		void FeedTasks(std::vector<Task>& tasks);
		void FeedTaskFastPath(Task&& task);
		
		Array _threads;
		std::atomic<uint32> _resigned;
		
		stl::ring_buffer<Task> _tasks;
		
		SpinLock _batchLock;
		std::deque<Batch *> _batchPool;
		
		std::mutex _workMutex;
		std::mutex _teardownMutex;
		std::mutex _consumerMutex;
		
		std::condition_variable _workAvailableCondition;
		std::condition_variable _teardownCondition;
		std::condition_variable _consumerCondition;
	};
}

#endif /* __RAYNE_THREADPOOL_H__ */
