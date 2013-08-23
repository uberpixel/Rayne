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
		
		RNAPI int32 GetAvailableConcurrency();
		RNAPI int32 GetBaseConcurrency() const { return _baseConcurrency; }
		
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
			
			size_t GetTaskCount() const { return _tasks.size(); }
			
		private:
			Batch(ThreadPool *pool) :
				_openTasks(0)
			{
				_pool = pool;
				_listener = 1;
				_commited = false;
			}
			
			ThreadPool *_pool;
			
			bool _commited;
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
			
			std::vector<Task> tasks;
			tasks.push_back(std::move(temp));
			
			FeedTasks(tasks);
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
			
			std::vector<Task> tasks;
			tasks.push_back(std::move(temp));
			
			FeedTasks(tasks);
			
			return result;
		}
		
		Batch *CreateBatch();
		
	private:		
		class Task
		{
		public:
			Task()
			{}
			
			Function function;
			Batch *batch;
		};
		
		struct ThreadContext
		{
			ThreadContext(size_t size) :
				hose(size)
			{}
			
			stl::ring_buffer<Task> hose;
			std::mutex lock;
			std::condition_variable condition;
		};
		
		Thread *CreateThread(size_t index);
		
		void Consumer();
		void FeedTasks(std::vector<Task>& tasks);
		
		Array _threads;
		size_t _threadCount;
		
		std::atomic<uint32> _resigned;
		std::mutex _teardownLock;
		std::condition_variable _teardownCondition;
		
		std::vector<ThreadContext *> _threadData;
		
		std::mutex _feederLock;
		std::condition_variable _feederCondition;
	};
}

#endif /* __RAYNE_THREADPOOL_H__ */
