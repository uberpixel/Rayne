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

namespace RN
{
	template<typename F>
	class ThreadPool;
	
	class ThreadCoordinator : public Singleton<ThreadCoordinator>
	{
	friend class Thread;
	public:
		ThreadCoordinator();
		
		machine_int AvailableConcurrency();
		machine_int BaseConcurrency() const { return _baseConcurrency; }
		
		ThreadPool<std::function<void ()>> *GlobalPool();
		
	private:
		void ConsumeConcurrency();
		void RestoreConcurrency();
		
		SpinLock _lock;
		
		machine_int _baseConcurrency;
		machine_int _consumedConcurrency;
		
		ThreadPool<std::function<void ()>> *_threadPool;
	};
	
	template<typename F>
	class ThreadPool
	{
	public:
		typedef enum
		{
			PoolTypeSerial,
			PoolTypeConcurrent
		} PoolType;
		
		ThreadPool(PoolType type)
		{
			_type = type;
			_running = 0;
			
			switch(_type)
			{
				case PoolTypeSerial:
				{
					Thread *thread = new Thread(std::bind(&ThreadPool::Consumer, this));
					_threads.AddObject(thread->Autorelease<Thread>());
					
					break;
				}
					
				case PoolTypeConcurrent:
				{
					machine_uint concurrency = ThreadCoordinator::SharedInstance()->BaseConcurrency();
					
					for(machine_uint i=0; i<concurrency; i++)
					{
						Thread *thread = new Thread(std::bind(&ThreadPool::Consumer, this));
						_threads.AddObject(thread->Autorelease<Thread>());
					}
					
					break;
				}
			}
		}
		
		virtual ~ThreadPool()
		{
			for(machine_uint i=0; i<_threads.Count(); i++)
			{
				Thread *thread = _threads.ObjectAtIndex(i);
				thread->Cancel();
			}
			
			WaitForTasksToComplete();
			_threads.RemoveAllObjects();
		}
		
		void AddTask(F&& task)
		{
			std::unique_lock<std::mutex> lock(_taskMutex);
			_tasks.AddObject(task);
			
			_taskCondition.notify_one();
		}
		
		void WaitForTasksToComplete()
		{
			std::unique_lock<std::mutex> lock(_waitMutex);
			_waitCondition.wait(lock, [&]() { return (_tasks.Count() == 0 && _running == 0); });
		}
		
	protected:
		void Consumer()
		{
			std::unique_lock<std::mutex> lock(_taskMutex);
			Thread *thread = Thread::CurrentThread();
			Context *context = new Context(Kernel::SharedInstance()->Context());
			
			while(1)
			{
				_taskCondition.wait(lock, [&]() { return (_tasks.Count() > 0 || thread->IsCancelled()); });
				
				if(thread->IsCancelled())
					break;
				
				
				F task = _tasks.LastObject();
				_tasks.RemoveLastObject();
				
				_running ++;
				lock.unlock();
				
				task();
				
				lock.lock();
				
				_running --;
				_waitCondition.notify_all();
			}
			
			delete context;
		}
		
	private:
		PoolType _type;
		
		Array<F> _tasks;
		Array<Thread> _threads;
		uint32 _running;
		
		std::condition_variable _taskCondition;
		std::condition_variable _waitCondition;
		std::mutex _taskMutex;
		std::mutex _waitMutex;
	};
}

#endif /* __RAYNE_THREADPOOL_H__ */
