//
//  RNThreadPool.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNThreadPool.h"
#include "RNKernel.h"

#define kRNThreadPoolTasksBuffer       4096
#define kRNThreadPoolLocalQueueMaxSize 40

namespace RN
{
	ThreadCoordinator::ThreadCoordinator()
	{
		// Sid:
		// Even on non multicore systems we have a base concurrency of at least two threads
		// This means that even on single core CPUs we spawn up to three threads in the base
		// implementation using a thread pool with threads equal to the amount of concurrency available.
		// I'm not sure if that's a good idea, and I lack the hardware to test it...
		_baseConcurrency = std::max(static_cast<int32>(std::thread::hardware_concurrency()), 2);
		_consumedConcurrency = 0;
	}
	
	void ThreadCoordinator::ConsumeConcurrency()
	{
		_consumedConcurrency ++;
	}
	
	void ThreadCoordinator::RestoreConcurrency()
	{
		_consumedConcurrency --;
	}
	
	int32 ThreadCoordinator::AvailableConcurrency()
	{
		int32 concurrency = _baseConcurrency - _consumedConcurrency.load();
		return concurrency;
	}
	
	// ---------------------
	// MARK: -
	// MARK: Thread Pool
	// ---------------------
	
	
	ThreadPool::ThreadPool(size_t maxJobs, size_t maxThreads) :
		_tasks(maxJobs != 0 ? maxJobs : kRNThreadPoolTasksBuffer)
	{
		_resigned.store(0);
		
		if(!maxThreads)
			maxThreads = ThreadCoordinator::SharedInstance()->BaseConcurrency();
		
		for(size_t i=0; i<maxThreads; i++)
		{
			Thread *thread = CreateThread();
			thread->Detach();
		}
	}
	
	ThreadPool::~ThreadPool()
	{
		uint32 toResign = static_cast<uint32>(_threads.Count());
		
		for(uint32 i=0; i<toResign; i++)
		{
			Thread *thread = _threads.ObjectAtIndex<Thread>(i);
			thread->Cancel();
		}
		
		std::unique_lock<std::mutex> lock(_teardownMutex);
		_teardownCondition.wait(lock, [&]{ return (_resigned.load() == toResign); });
	}
	
	
	Thread *ThreadPool::CreateThread()
	{
		Thread *thread = new Thread(std::bind(&ThreadPool::Consumer, this), false);
		_threads.AddObject(thread);
		
		return thread->Autorelease();
	}
	
	
	
	ThreadPool::Batch ThreadPool::OpenBatch()
	{
		Batch batch(new __Batch(this));
		return batch;
	}
	

	
	void ThreadPool::FeedTasks(std::vector<Task>& tasks)
	{
		size_t toWrite = tasks.size();
		size_t offset  = 0;
		
		while(toWrite > 0)
		{
			std::unique_lock<std::mutex> feedlock(_workMutex);
			
			if(_tasks.size() == _tasks.capacity())
			{
				feedlock.unlock();
				
				std::unique_lock<std::mutex> lock(_consumerMutex);
				_consumerCondition.wait(lock);
				
				continue;
			}
			
			size_t pushable = std::min(_tasks.capacity() - _tasks.size(), toWrite);
			toWrite -= pushable;
			
			for(size_t i=0; i<pushable; i++)
			{
				_tasks.push(std::move(tasks[offset + i]));
			}
			
			offset += pushable;
			_workAvailableCondition.notify_all();
		}
	}
	
	void ThreadPool::ReadTasks(std::vector<Task>& tasks)
	{
		std::unique_lock<std::mutex> lock(_workMutex);
		_workAvailableCondition.wait(lock, [&]{ return (_tasks.size() > 0); });
		
		size_t move = std::min<size_t>(kRNThreadPoolLocalQueueMaxSize, std::max<size_t>(1, _tasks.size() / _threads.Count()));
		tasks.reserve(move);
		
		for(size_t i=0; i<move; i++)
		{
			tasks.push_back(std::move(_tasks.front()));
			_tasks.pop();
		}
		
		_consumerCondition.notify_one();
	}
	
	void ThreadPool::Consumer()
	{
		Thread *thread = Thread::CurrentThread();
		Context *context = new Context(Kernel::SharedInstance()->Context());
		AutoreleasePool *pool = new AutoreleasePool();
		
		context->MakeActiveContext();
		
		while(!thread->IsCancelled())
		{
			std::vector<Task> tasks;
			ReadTasks(tasks);
			
			size_t size = tasks.size();
			
			for(size_t i=0; i<size; i++)
			{
				Task& task = tasks[i];
				
				task.function();
				
				if(Batch batch = task.batch.lock())
				{
					uint32 tasksLeft = batch->_openTasks.fetch_sub(1);
					
					if(tasksLeft == 1)
					{
						std::lock_guard<std::mutex> lock(batch->_lock);
						batch->_waitCondition.notify_all();
					}
				}
			}
		}
		
		context->DeactivateContext();
		delete context;
		delete pool;
		
		_resigned ++;
		_teardownCondition.notify_one();
	}
	
	// ---------------------
	// MARK: -
	// MARK: Batch
	// ---------------------
	
	void ThreadPool::__Batch::Commit()
	{
		_openTasks.store(static_cast<uint32>(_tasks.size()));
		_pool->FeedTasks(_tasks);
	}
	
	void ThreadPool::__Batch::Wait()
	{
		std::unique_lock<std::mutex> lock(_lock);
		_waitCondition.wait(lock, [&]{ return (_openTasks.load() == 0); });
	}
}
