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
#include "RNFunction.h"
#include "RNArray.h"
#include "RNContext.h"
#include "RNKernel.h"

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
		
		RNAPI ThreadPool *GlobalPool();
		
	private:
		void ConsumeConcurrency();
		void RestoreConcurrency();
		
		SpinLock _lock;
		
		machine_int _baseConcurrency;
		machine_int _consumedConcurrency;
		
		ThreadPool *_globalPool;
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
			_insertingTaskBatch = false;
			
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
			
			_waitCondition.notify_all(); // Notify all sleeping threads
			
			std::unique_lock<std::mutex> lock(_tearDownMutex);
			_tearDownCondition.wait(lock, [&]() { return (_resigned == _threads.Count()); } );
		}
		
		template<typename F>
		std::future<typename std::result_of<F()>::type> AddTask(F f)
		{
			typedef typename std::result_of<F()>::type resultType;
			
			std::packaged_task<resultType()> task(std::move(f));
			std::future<resultType> result(task.get_future());
			
			if(!_insertingTaskBatch)
			{
				_lock.Lock();
				_workQueue.push_back(std::move(task));
				_lock.Unlock();
				
				_waitCondition.notify_one();
			}
			else
			{
				_workQueue.push_back(std::move(task));
			}
			
			return result;
		}
		
		void BeginTaskBatch()
		{
			_lock.Lock();
			_insertingTaskBatch = true;
		}
		
		void EndTaskBatch()
		{
			_insertingTaskBatch = false;
			_lock.Unlock();
			
			_waitCondition.notify_all();
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
			std::deque<Function> localQueue;
			
			while(!thread->IsCancelled())
			{
				if(localQueue.size() == 0)
				{
					_lock.Lock();
					
					if(_workQueue.size() == 0)
					{
						// TODO: Check if we can steal tasks from other threads
						_lock.Unlock();
						
						std::unique_lock<std::mutex> lock(_waitMutex);
						_waitCondition.wait(lock); // Spurious wake ups are okay, the cost of a predicate is higher than looping again.
						continue;
					}
					
					machine_uint moveLocal = MIN(kRNThreadPoolLocalQueueMaxSize, _workQueue.size());
					std::deque<Function>::iterator last = _workQueue.begin();
					std::advance(last, moveLocal);
					
					std::move(_workQueue.begin(), last, std::back_inserter(localQueue));
					_workQueue.erase(_workQueue.begin(), last);

					_lock.Unlock();
				}
				
				Function task = std::move(localQueue.front());
				localQueue.pop_front();
				
				task();
			}
			
			_resigned ++;
			_tearDownCondition.notify_all();
		}
		
		SpinLock _lock;
		PoolType _type;
		
		bool _insertingTaskBatch;
		
		Array<Thread> _threads;
		machine_uint _resigned;
		std::deque<Function> _workQueue;
	
		std::mutex _tearDownMutex;
		std::mutex _waitMutex;
		std::condition_variable _tearDownCondition;
		std::condition_variable _waitCondition;
	};
}

#endif /* __RAYNE_THREADPOOL_H__ */
