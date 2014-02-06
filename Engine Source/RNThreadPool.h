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
		class Allocator
		{
		public:
			Allocator() {}
			virtual ~Allocator() {}
			
			virtual void *Allocate(size_t size) { return Memory::Allocate(size); }
			virtual void Free(void *ptr) { Memory::Free(ptr); }
			virtual void Evict() { }
			
			template<class T, class ... Args>
			T *New(Args&&... args)
			{
				void *buffer = Allocate(sizeof(T));
				T *temp = new(buffer) T(std::forward<Args>(args)...);
				
				return temp;
			}
			
			template<class T>
			void Delete(T *ptr)
			{
				if(ptr)
				{
					ptr->~T();
					Free(ptr);
				}
			}
		};
		
		class SmallObjectsAllocator : public Allocator
		{
		public:
			
			void *Allocate(size_t size) final { return _pool.Allocate(size); }
			void Free(void *ptr) final { }
			void Evict() { _pool.Evict(); }
			
		private:
			Memory::Pool _pool;
		};
		
		class Batch
		{
		friend class ThreadPool;
		public:
			template<class F>
			void AddTask(F&& f)
			{
				 Task temp(std::move(f), &_allocator, this);
				_tasks.push_back(std::move(temp));
			}
			
			template<class F>
			std::future<typename std::result_of<F()>::type> AddTaskWithFuture(F&& f)
			{
				typedef typename std::result_of<F()>::type resultType;
				
				std::packaged_task<resultType ()> task(std::move(f));
				std::future<resultType> result(task.get_future());
				
				Task temp(std::move(task), &_allocator, this);
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
			
			RNAPI size_t GetTaskCount() const { return _tasks.size(); }
			
		private:
			Batch(Allocator& allocator, ThreadPool *pool) :
				_allocator(allocator),
				_openTasks(0),
				_pool(pool)
			{
				_listener = 1;
				_commited = false;
			}
			
			~Batch()
			{
				_allocator.Evict();
			}
			
			ThreadPool *_pool;
			
			bool _commited;
			std::vector<Task> _tasks;
			std::atomic<uint32> _openTasks;
			std::atomic<uint32> _listener;
			
			std::mutex _lock;
			std::condition_variable _waitCondition;
			Allocator& _allocator;
		};
		
		friend class Batch;
		
		RNAPI ThreadPool(size_t maxThreads=0);
		RNAPI ~ThreadPool() override;
		
		template<class F>
		void AddTask(F&& f)
		{
			Task temp(std::move(f), &_allocator, nullptr);
			
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
			
			Task temp(std::move(task), &_allocator, nullptr);

			std::vector<Task> tasks;
			tasks.push_back(std::move(temp));
			
			FeedTasks(tasks);
			
			return result;
		}
		
		RNAPI Batch *CreateBatch();
		RNAPI Batch *CreateBatch(Allocator& allocator);
		RNAPI Allocator& GetDefaultAllocator() { return _allocator; }
		
	private:
		class Function
		{
		public:
			Function()
			{
				_allocator = nullptr;
				_implementation = nullptr;
			}
			
			template<typename F>
			Function(F&& f, Allocator *allocator) :
				_allocator(allocator)
			{
				_implementation = allocator->New<ImplementationType<F>>(std::move(f));
			}
			
			Function(Function&& other)
			{
				_implementation = std::move(other._implementation);
				_allocator = other._allocator;
				
				other._implementation = nullptr;
				other._allocator = nullptr;
			}
			
			Function& operator=(Function&& other)
			{
				_implementation = std::move(other._implementation);
				_allocator = other._allocator;
				
				other._implementation = nullptr;
				other._allocator = nullptr;
				
				return *this;
			}
			
			~Function()
			{
				if(_allocator)
					_allocator->Delete(_implementation);
			}
			
			Function(const Function&) = delete;
			Function& operator= (const Function&) = delete;
			
			void operator() () { _implementation->Call(); }
			
		private:
			struct Base
			{
				virtual void Call() = 0;
				virtual ~Base() {}
			};
			
			template<typename F>
			struct ImplementationType : Base
			{
				ImplementationType(F&& f) :
					function(std::move(f))
				{}
				
				void Call()
				{
					function();
				}
				
				F function;
			};
			
			Allocator *_allocator;
			Base *_implementation;
		};
		
		class Task
		{
		public:
			Task() = default;
			
			template<typename F>
			Task(F&& f, Allocator *allocator, Batch *tbatch) :
				function(std::move(f), allocator),
				batch(tbatch)
			{}

			Task(Task&& other) :
				function(std::move(other.function)),
				batch(std::move(other.batch))
			{}

			Task& operator= (Task&& other)
			{
				function = std::move(other.function);
				batch = std::move(other.batch);

				return *this;
			}
			
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
		Allocator _allocator;
		
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
