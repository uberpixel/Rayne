//
//  RNThreadPool.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_THREADPOOL_H__
#define __RAYNE_THREADPOOL_H__

#include "RNBase.h"
#include "RNThread.h"
#include "RNArray.h"
#include "RNAutoreleasePool.h"
#include "RNRingbuffer.h"

namespace RN
{
	class ThreadPool;
	
	class ThreadCoordinator : public ISingleton<ThreadCoordinator>
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
		
		RNDeclareSingleton(ThreadCoordinator)
	};
	
	class ThreadPool : public ISingleton<ThreadPool>
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
				_tasks.emplace_back(std::move(f), this);
			}
			
			template<class F>
			std::future<typename std::result_of<F()>::type> AddTaskWithFuture(F&& f)
			{
				typedef typename std::result_of<F()>::type resultType;
				
				std::packaged_task<resultType ()> task(std::move(f));
				std::future<resultType> result(task.get_future());
				
				_tasks.emplace_back(std::move(task), this);
				
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
			
			RNAPI size_t GetTaskCount() const { return _tasks.size(); }
			
		private:
			Batch(ThreadPool *pool) :
				_openTasks(0),
				_pool(pool)
			{
				_listener = 1;
				_commited = false;
			}
			
			~Batch()
			{}
			
			ThreadPool *_pool;
			
			bool _commited;
			std::vector<Task> _tasks;
			std::atomic<uint32> _openTasks;
			std::atomic<uint32> _listener;
			
			std::mutex _lock;
			std::condition_variable _waitCondition;
		};
		
		friend class Batch;
		
		RNAPI ThreadPool(size_t maxThreads=0);
		RNAPI ~ThreadPool() override;
		
		template<class F>
		void AddTask(F&& f)
		{
			std::vector<Task> tasks;
			tasks.emplace_back(std::move(f), nullptr);
			
			FeedTasks(tasks);
		}
		
		template<class F>
		std::future<typename std::result_of<F()>::type> AddTaskWithFuture(F&& f)
		{
			typedef typename std::result_of<F()>::type resultType;
			
			std::packaged_task<resultType ()> task(std::move(f));
			std::future<resultType> result(task.get_future());
			
			std::vector<Task> tasks;
			tasks.emplace_back(std::move(task), nullptr);
			
			FeedTasks(tasks);
			
			return result;
		}
		
		RNAPI Batch *CreateBatch();
		
	private:
		class Task
		{
		public:
			Task() = default;
			
			template<typename F>
			Task(F&& f, Batch *tbatch) :
				function(std::move(f)),
				batch(tbatch)
			{}

			Task(Task&& other) :
				function(std::move(other.function)),
				batch(std::move(other.batch))
			{}

			Task& operator= (Task&& other)
			{
				function = std::move(other.function);
				batch    = std::move(other.batch);

				return *this;
			}
			
			Task(const Task &) = delete;
			Task &operator =(Task &) = delete;
			
			Function function;
			Batch *batch;
		};
		
		struct ThreadContext
		{
			stl::lock_free_ring_buffer<Task, 2048> hose;
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
		std::mutex _consumerLock;
		std::mutex _feedLock;
		std::mutex _syncLock;
		
		std::condition_variable _feederCondition;
		std::condition_variable _consumerCondition;
		std::condition_variable _syncpoint;
		
		std::atomic<bool> _running;
		
		RNDeclareSingleton(ThreadPool)
	};
}

#endif /* __RAYNE_THREADPOOL_H__ */
