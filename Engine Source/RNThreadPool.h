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

#define kRNThreadPoolLocalQueueMaxSize 25

namespace RN
{
	class ThreadPool;
	
	class ThreadCoordinator : public Singleton<ThreadCoordinator>
	{
	friend class Thread;
	public:
		RNAPI ThreadCoordinator();
		
		RNAPI machine_int AvailableConcurrency();
		RNAPI machine_int BaseConcurrency() const { return _baseConcurrency; }
		
	private:
		void ConsumeConcurrency();
		void RestoreConcurrency();
		
		machine_int _baseConcurrency;
		std::atomic<machine_int> _consumedConcurrency;
	};
	
	class ThreadPool : public Singleton<ThreadPool>
	{
	class Task;
	public:
		class __Batch : public std::enable_shared_from_this<__Batch>
		{
		friend class ThreadPool;
		public:
			template<class F>
			void AddTask(F&& f)
			{
				Task temp;
				temp.function = std::move(f);
				temp.batch    = SharedPointer();
				
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
				temp.batch    = SharedPointer();
				
				_tasks.push_back(std::move(temp));
				
				return result;
			}
			
			void Commit();
			void Wait();
			
		private:
			__Batch(ThreadPool *pool) :
				_openTasks(0)
			{
				_pool = pool;
			}
			
			std::shared_ptr<__Batch> SharedPointer()
			{
				return shared_from_this();
			}
			
			ThreadPool *_pool;
			
			std::vector<Task> _tasks;
			std::atomic<uint32> _openTasks;
			
			std::mutex _lock;
			std::condition_variable _waitCondition;
		};
		typedef std::shared_ptr<__Batch> Batch;
		friend class __Batch;
		
		
		ThreadPool(machine_uint maxJobs=0, machine_uint maxThreads=0);
		~ThreadPool() override;
		
		Batch OpenBatch();
		
		
	private:		
		class Task
		{
		public:
			Task()
			{}
			
			Function function;
			std::weak_ptr<__Batch> batch;
		};
		
		Thread *CreateThread();
		
		Task ReadTask();
		void Consumer();
		void FeedTasks(std::vector<Task>& tasks);
		
		Array _threads;
		std::atomic<uint32> _resigned;
		
		stl::ring_buffer<Task> _tasks;
		
		std::mutex _workMutex;
		std::mutex _teardownMutex;
		std::mutex _consumerMutex;
		
		std::condition_variable _workAvailableCondition;
		std::condition_variable _teardownCondition;
		std::condition_variable _consumerCondition;
	};
}

#endif /* __RAYNE_THREADPOOL_H__ */
