//
//  RNThreadPool.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_THREADPOOL_H__
#define __RAYNE_THREADPOOL_H__

#include "RNBase.h"
#include "RNThread.h"
#include "RNSpinLock.h"
#include "RNArray.h"
#include "RNContext.h"
#include "RNKernel.h"

#include <list>
#include <future>

#define kRNThreadConsumerKey "kRNThreadConsumer"

namespace RN
{
	class ThreadPool;
	
	class ThreadCoordinator : public Singleton<ThreadCoordinator>
	{
	friend class Thread;
	public:
		ThreadCoordinator();
		
		machine_int AvailableConcurrency();
		machine_int BaseConcurrency() const { return _baseConcurrency; }
		
		ThreadPool *GlobalPool();
		
	private:
		void ConsumeConcurrency();
		void RestoreConcurrency();
		
		SpinLock _lock;
		
		machine_int _baseConcurrency;
		machine_int _consumedConcurrency;
		
		ThreadPool *_globalPool;
	};
	
	class FunctionWrapper
	{
	public:
		template<typename F>
		FunctionWrapper(F&& f) :
			_implementation(new ImplementationType<F>(std::move(f)))
		{}
		
		void operator()() { _implementation->Call(); }
		
		FunctionWrapper() = default;
		FunctionWrapper(FunctionWrapper&& other) :
			_implementation(std::move(other._implementation))
		{}
		
		FunctionWrapper& operator=(FunctionWrapper&& other)
		{
			_implementation = std::move(other._implementation);
			return *this;
		}
		
		FunctionWrapper(const FunctionWrapper&) = delete;
		FunctionWrapper(FunctionWrapper&) = delete;
		FunctionWrapper& operator= (const FunctionWrapper&) = delete;
		
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
		
		std::unique_ptr<Base> _implementation;
	};
	
	class ThreadPool
	{
	public:
		typedef enum
		{
			PoolTypeSerial,
			PoolTypeConcurrent
		} PoolType;
		
		ThreadPool(PoolType type, machine_uint maxThreads=0)
		{
			_type = type;
			_resigned = 0;
			
			switch(_type)
			{
				case PoolTypeSerial:
				{
					Thread *thread = CreateThread();
					thread->Detach();
					break;
				}
					
				case PoolTypeConcurrent:
				{
					if(maxThreads == 0)
						maxThreads = ThreadCoordinator::SharedInstance()->BaseConcurrency();
					
					for(machine_uint i=0; i<maxThreads; i++)
					{
						Thread *thread = CreateThread();
						thread->Detach();
					}
					
					break;
				}
			}
		}
		
		~ThreadPool()
		{
			for(machine_uint i=0; i<_threads.Count(); i++)
			{
				Thread *thread = _threads.ObjectAtIndex(i);
				thread->Cancel();
			}
			
			std::unique_lock<std::mutex> lock(_tearDownMutex);
			_tearDownCondition.wait(lock, [&]() { return (_resigned == _threads.Count()); } );
		}
		
		template<typename F>
		std::future<typename std::result_of<F()>::type> AddTask(F f)
		{
			typedef typename std::result_of<F()>::type resultType;
			
			std::packaged_task<resultType()> task(std::move(f));
			std::future<resultType> result(task.get_future());
			
			_lock.Lock();
			_workQueue.push_back(std::move(task));
			_lock.Unlock();
			
			_waitCondition.notify_one();
			return result;
		}
		
	private:
		Thread *CreateThread()
		{
			Thread *thread = new Thread(std::bind(&ThreadPool::Consumer, this), false);
			_threads.AddObject(thread);
			
			return thread;
		}
		
		void Consumer()
		{
			Thread *thread = Thread::CurrentThread();
			
			while(!thread->IsCancelled())
			{
				if(_lock.TryLock())
				{
					if(_workQueue.size() == 0)
					{
						_lock.Unlock();
						
						std::unique_lock<std::mutex> lock(_waitMutex);
						_waitCondition.wait(lock); // Spurios wake ups are okay
						continue;
					}
					
					FunctionWrapper task = std::move(_workQueue.front());
					_workQueue.pop_front();
					
					_lock.Unlock();
					
					task();
				}
				else
				{
					std::this_thread::yield();
				}
			}
			
			_resigned ++;
			_tearDownCondition.notify_all();
		}
		
		SpinLock _lock;
		PoolType _type;
		
		Array<Thread> _threads;
		machine_uint _resigned;
		std::list<FunctionWrapper> _workQueue;
	
		std::mutex _tearDownMutex;
		std::mutex _waitMutex;
		std::condition_variable _tearDownCondition;
		std::condition_variable _waitCondition;
	};
}

#endif /* __RAYNE_THREADPOOL_H__ */
