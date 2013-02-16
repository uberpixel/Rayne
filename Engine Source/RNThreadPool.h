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

namespace RN
{
	class ThreadPool : public Singleton<ThreadPool>
	{
		friend class Thread;
	public:
		ThreadPool();
		
		machine_int AvailableConcurrency();
		
	private:
		void ConsumeConcurrency();
		void RestoreConcurrency();
		
		SpinLock _lock;
		
		machine_int _baseConcurrency;
		machine_int _consumedConcurrency;
	};
	
	
	template<typename F>
	class WorkerPool
	{
	public:
		typedef enum
		{
			PoolTypeSerial,
			PoolTypeConcurrent
		} PoolType;
		
		
		WorkerPool(PoolType type)
		{
			_type = type;
		}
		
		~WorkerPool()
		{}
		
		
		void AddTask(F&& task)
		{
			std::unique_lock<std::mutex> lock(_mutex);
			_tasks.AddObject(task);
			
			bool spinUpThread = false;
			switch(_type)
			{
				case PoolTypeSerial:
					spinUpThread = (_threads.Count() == 0);
					break;
					
				case PoolTypeConcurrent:
					spinUpThread = (ThreadPool::SharedInstance()->AvailableConcurrency() > 0 || _threads.Count() == 0);
					break;
			}
			
			if(spinUpThread)
			{
				Thread *thread = new Thread(std::bind(&WorkerPool::Consumer, this));
				_threads.AddObject(thread->Autorelease<Thread>());
			}
		}
		
		void WaitForTasksToComplete()
		{
			std::unique_lock<std::mutex> lock(_mutex);
			_waitCondition.wait(lock, [&]() { return (_tasks.Count() == 0 && _threads.Count() == 0); });
		}
		
	private:
		void Consumer()
		{
			std::unique_lock<std::mutex> lock(_mutex);
			machine_uint workLeft = _tasks.Count();
			
			while(workLeft > 0)
			{
				F task = _tasks.ObjectAtIndex(0);
				_tasks.RemoveObjectAtIndex(0);
				
				lock.unlock();
				
				task();
				
				lock.lock();
				workLeft = _tasks.Count();
			}
			
			_threads.RemoveObject(Thread::CurrentThread());
			printf("Consumer finished!\n");
			_waitCondition.notify_all();
		}
		
		PoolType _type;
		
		Array<F> _tasks;
		Array<Thread> _threads;
		
		std::condition_variable _waitCondition;
		std::mutex _mutex;
	};
}

#endif /* __RAYNE_THREADPOOL_H__ */
