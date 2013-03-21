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

#define kRNThreadConsumerKey "kRNThreadConsumer"

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
					Thread *thread = CreateThread();
					thread->Detach();
					break;
				}
					
				case PoolTypeConcurrent:
				{
					machine_uint concurrency = ThreadCoordinator::SharedInstance()->BaseConcurrency();
					
					for(machine_uint i=0; i<concurrency; i++)
					{
						Thread *thread = CreateThread();
						thread->Detach();
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
			Thread *candidate = 0;
			ThreadConsumer *candidateConsumer = 0;
			
			machine_uint leastTasks = 0;
			
			switch(_type)
			{
				case PoolTypeSerial:
					candidate = _threads.ObjectAtIndex(0);
					candidateConsumer = candidate->ObjectForKey<ThreadConsumer>(kRNThreadConsumerKey);
					break;
					
				case PoolTypeConcurrent:
				{
					machine_uint threads = _threads.Count();
					for(machine_uint i=0; i<threads; i++)
					{
						Thread *thread = _threads.ObjectAtIndex(i);
						ThreadConsumer *consumer = thread->ObjectForKey<ThreadConsumer>(kRNThreadConsumerKey);
						consumer->lock.Lock();
						
						machine_uint tasks = consumer->tasks.Count();
						if(tasks == 0)
						{
							consumer->lock.Unlock();
							candidate = thread;
							candidateConsumer = consumer;
							
							break;
						}
						
						if(tasks < leastTasks || leastTasks == 0)
						{
							candidate = thread;
							candidateConsumer = consumer;
						}
						
						consumer->lock.Unlock();
					}
					
					break;
				}
			}
			
			RN_ASSERT0(candidate && candidateConsumer);
			
			candidateConsumer->lock.Lock();
			
			if(!candidateConsumer->running)
			{
				std::unique_lock<std::mutex> consumerLock(candidateConsumer->mutex);
				std::unique_lock<std::mutex> lock(_waitMutex);
				_running ++;
				lock.unlock();
				
				candidateConsumer->tasks.AddObject(task);
				candidateConsumer->running = true;
				candidateConsumer->lock.Unlock();
				
				consumerLock.unlock();
				candidateConsumer->condition.notify_one();
			}
			else
			{
				candidateConsumer->tasks.AddObject(task);
				candidateConsumer->lock.Unlock();
			}
		}
		
		void WaitForTasksToComplete()
		{
			std::unique_lock<std::mutex> lock(_waitMutex);
			_waitCondition.wait(lock, [&]() { return (_running == 0); });
		}
		
	private:
		struct ThreadConsumer
		{
			std::mutex mutex;
			std::condition_variable condition;
			
			Array<F> tasks;
			SpinLock lock;
			
			bool running;
		};
		
		Thread *CreateThread()
		{
			Thread *thread = new Thread(std::bind(&ThreadPool::Consumer, this), false);
			ThreadConsumer *consumer = new ThreadConsumer();
			
			consumer->running = false;
			
			thread->SetObjectForKey<ThreadConsumer>(consumer, kRNThreadConsumerKey);
			thread->Autorelease();
			
			_threads.AddObject(thread);
			
			return thread;
		}
		
		void Consumer()
		{
			Thread *thread = Thread::CurrentThread();
			ThreadConsumer *consumer = thread->ObjectForKey<ThreadConsumer>(kRNThreadConsumerKey);
			
			/*Context *context = new Context(Kernel::SharedInstance()->Context());
			context->MakeActiveContext();*/
			
			bool hasTasks = false;
			
			while(1)
			{
				bool wasRunning = consumer->running;
				
				do {
					consumer->lock.Lock();
					hasTasks = (consumer->tasks.Count() > 0);
					
					if(!hasTasks)
					{						
						if(wasRunning)
						{
							std::unique_lock<std::mutex> waitLock(_waitMutex);
							_running --;
							waitLock.unlock();
							
							_waitCondition.notify_all();
							wasRunning = false;
						}
						
						consumer->running = false;
						consumer->lock.Unlock();
						
						std::unique_lock<std::mutex> lock(consumer->mutex);
						consumer->condition.wait(lock, [&]() { return (consumer->tasks.Count() > 0 || thread->IsCancelled()); });
						
						if(thread->IsCancelled())
						{
							delete consumer;
							// delete context;
							
							return;
						}
						
						consumer->lock.Lock();
						hasTasks = (consumer->tasks.Count() > 0);
					}
					
				} while(!hasTasks);
				
				F task = consumer->tasks.LastObject();				
				consumer->tasks.RemoveLastObject();
				
				consumer->running = true;
				consumer->lock.Unlock();
				
				task();
			}
		}
		
		PoolType _type;
		Array<Thread> _threads;
		
		std::condition_variable _waitCondition;
		std::mutex _waitMutex;
		uint32 _running;
	};
}

#endif /* __RAYNE_THREADPOOL_H__ */
