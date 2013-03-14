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
	class ThreadCoordinator : public Singleton<ThreadCoordinator>
	{
		friend class Thread;
	public:
		ThreadCoordinator();
		
		machine_int AvailableConcurrency();
		
	private:
		void ConsumeConcurrency();
		void RestoreConcurrency();
		
		SpinLock _lock;
		
		machine_int _baseConcurrency;
		machine_int _consumedConcurrency;
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
		}
		
		virtual ~ThreadPool()
		{
			WaitForTasksToComplete();
		}
		
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
					spinUpThread = (ThreadCoordinator::SharedInstance()->AvailableConcurrency() > 0 || _threads.Count() == 0);
					break;
			}
			
			if(spinUpThread)
			{
				Thread *thread = new Thread(std::bind(&ThreadPool::Consumer, this));
				_threads.AddObject(thread->Autorelease<Thread>());
			}
		}
		
		void WaitForTasksToComplete()
		{
			std::unique_lock<std::mutex> lock(_mutex);
			_waitCondition.wait(lock, [&]() { return (_tasks.Count() == 0 && _threads.Count() == 0); });
		}
		
	protected:
		virtual void Consumer()
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
				
				if(_threads.Count() > 1 && ThreadCoordinator::SharedInstance()->AvailableConcurrency() < 0)
					break;
				
				workLeft = _tasks.Count();
			}
			
			_threads.RemoveObject(Thread::CurrentThread());
			_waitCondition.notify_all();
		}
		
	private:
		PoolType _type;
		
		Array<F> _tasks;
		Array<Thread> _threads;
		
		std::condition_variable _waitCondition;
		std::mutex _mutex;
	};
	
	template<typename F>
	class OpenGLThreadPool : public ThreadPool<F>
	{
	public:
		OpenGLThreadPool() :
			ThreadPool<F>(ThreadPool<F>::PoolTypeSerial)
		{
			_context = new Context(Kernel::SharedInstance()->Context());
		}
		
		virtual ~OpenGLThreadPool()
		{
			ThreadPool<F>::WaitForTasksToComplete();
			_context->Autorelease();
		}
		
	private:
		virtual void Consumer()
		{
			_context->MakeActiveContext();
			
			ThreadPool<F>::Consumer();
			
			_context->DeactivateContext();
		}
		
		Context *_context;
	};
}

#endif /* __RAYNE_THREADPOOL_H__ */
