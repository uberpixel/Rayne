//
//  RNThreadPool.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNThreadPool.h"
#include "RNNumber.h"
#include "RNKernel.h"
#include "RNLogging.h"

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
	
	int32 ThreadCoordinator::GetAvailableConcurrency()
	{
		int32 concurrency = _baseConcurrency - _consumedConcurrency.load();
		return concurrency;
	}
	
	// ---------------------
	// MARK: -
	// MARK: Thread Pool
	// ---------------------
	
	
	ThreadPool::ThreadPool(size_t maxThreads)
	{
		_resigned.store(0);
		
		_threadCount = (maxThreads > 0) ? maxThreads : ThreadCoordinator::GetSharedInstance()->GetBaseConcurrency();
		
		for(size_t i = 0; i < _threadCount; i ++)
		{
			_threadData.push_back(new ThreadContext());
			
			Thread *thread = CreateThread(i);
			thread->Detach();
		}
	}
	
	ThreadPool::~ThreadPool()
	{
		uint32 toResign = static_cast<uint32>(_threads.GetCount());
		
		for(uint32 i = 0; i < toResign; i ++)
		{
			Thread *thread = _threads.GetObjectAtIndex<Thread>(i);
			thread->Cancel();
		}
		
		std::unique_lock<std::mutex> lock(_teardownLock);
		_teardownCondition.wait(lock, [&]{ return (_resigned.load() == _threadCount); });
		
		for(ThreadContext *context : _threadData)
			delete context;
	}
	
	
	Thread *ThreadPool::CreateThread(size_t index)
	{
		Thread *thread = new Thread(std::bind(&ThreadPool::Consumer, this), false);
		_threads.AddObject(thread);
		
		thread->SetObjectForKey(Number::WithUint32(static_cast<uint32>(index)), RNCSTR("__kRNThreadID"));
		
		return thread->Autorelease();
	}
	
	
	ThreadPool::Batch *ThreadPool::CreateBatch()
	{
		Batch *batch = new Batch(GetDefaultAllocator(), this);
		return batch;
	}
	
	ThreadPool::Batch *ThreadPool::CreateBatch(Allocator& allocator)
	{
		Batch *batch = new Batch(allocator, this);
		return batch;
	}
	

	
	void ThreadPool::FeedTasks(std::vector<Task>& tasks)
	{
		size_t toWrite = tasks.size();
		size_t offset  = 0;
		
		while(toWrite > 0)
		{
			size_t perThread = toWrite / _threadCount;
			bool written = false;
			
			if(!perThread)
				perThread = toWrite;
			
			
			for(size_t i = 0; (i < _threadCount && toWrite > 0); i ++)
			{
				ThreadContext *context = _threadData[i];
				size_t pushed;
				
				for(pushed = 0; pushed < perThread; pushed ++)
				{
					if(!context->hose.push(std::move(tasks[offset])))
						break;
					
					offset ++;
				}
				
				if(pushed > 0)
				{
					written = true;
					context->condition.notify_one();
				}
				
				toWrite -= pushed;
			}
			
			if(!written && toWrite > 0)
			{
				Log::Loggable loggable(Log::Level::Debug);
				loggable << "Thread pool left with " << toWrite << " of " << tasks.size() << " tasks";
				
				std::unique_lock<std::mutex> lock(_feederLock);
				_feederCondition.wait_for(lock, std::chrono::microseconds(500));
			}
		}
		
		tasks.clear();
	}
	
	void ThreadPool::Consumer()
	{
		Thread *thread = Thread::GetCurrentThread();
		Context *context = new Context(Kernel::GetSharedInstance()->GetContext());
		AutoreleasePool *pool = new AutoreleasePool();
		
		context->MakeActiveContext();
		
		size_t threadID = thread->GetObjectForKey<Number>(RNCSTR("__kRNThreadID"))->GetUint32Value();
		ThreadContext *local = _threadData[threadID];
		
		while(!thread->IsCancelled())
		{
			Task task;
			
			if(local->hose.pop(task))
			{
				task.function();
				
				if(task.batch)
				{
					if((-- task.batch->_openTasks) == 0)
					{
						std::unique_lock<std::mutex> lock(task.batch->_lock);
						
						task.batch->_waitCondition.notify_all();
						lock.unlock();
						
						task.batch->Release();
					}
				}
			}
			else
			{
				std::unique_lock<std::mutex> lock(local->lock);
				local->condition.wait(lock, [&]() { return (local->hose.was_empty() == false); });
			}

			_feederCondition.notify_one();
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
	
	void ThreadPool::Batch::Retain()
	{
		_listener ++;
	}
	
	void ThreadPool::Batch::Release()
	{
		if((-- _listener) == 0)
		{
			delete this;
		}
		
	}
	
	void ThreadPool::Batch::Commit()
	{
		RN_ASSERT(_commited == false, "A batch can only be committed once!");
		
		Retain();
		
		_openTasks.store(static_cast<uint32>(_tasks.size()));
		_pool->FeedTasks(_tasks);
		
		_commited = true;
	}
	
	void ThreadPool::Batch::Wait()
	{
		Retain();
		
		std::unique_lock<std::mutex> lock(_lock);
		
		if(_openTasks.load() > 0)
			_waitCondition.wait(lock, [&]{ return (_openTasks.load() == 0); });
		
		Release();
	}
}
