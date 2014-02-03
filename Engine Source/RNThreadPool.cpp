//
//  RNThreadPool.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNThreadPool.h"
#include "RNNumber.h"
#include "RNKernel.h"
#include "RNLogging.h"

namespace RN
{
	RNDeclareSingleton(ThreadCoordinator)
	RNDeclareSingleton(ThreadPool)
	
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
	
	
	ThreadPool::ThreadPool(size_t maxThreads) :
		_running(false),
		_resigned(0)
	{
		_threadCount = (maxThreads > 0) ? maxThreads : ThreadCoordinator::GetSharedInstance()->GetBaseConcurrency();
		
		for(size_t i = 0; i < _threadCount; i ++)
		{
			_threadData.push_back(new ThreadContext());
			
			Thread *thread = CreateThread(i);
			thread->Detach();
		}
		
		std::unique_lock<std::mutex> lock(_syncLock);
		_running.store(true);
		_syncpoint.notify_all();
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
		_feedLock.lock();
		
		size_t toWrite = tasks.size();
		size_t offset  = 0;
		
		while(toWrite > 0)
		{
			size_t perThread = toWrite / _threadCount;
			bool written = false;
			
			if(!perThread)
				perThread = toWrite;
			
			for(size_t i = 0; i < _threadCount; i ++)
			{
				ThreadContext *context = _threadData[i];
				size_t pushed = 0;
				
				perThread = std::min(perThread, toWrite);
				
				for(pushed = 0; pushed < perThread;)
				{
					if(!context->hose.push(std::move(tasks[offset])))
						break;
					
					offset ++;
					pushed ++;
				}
				
				written = (pushed > 0);
				toWrite -= pushed;
				
				if(toWrite == 0)
					break;
			}
			
			if(!written)
			{
				std::unique_lock<std::mutex> lock(_consumerLock);
				_consumerCondition.notify_all();
			}
			
			if(!written && toWrite > 0)
			{
				std::unique_lock<std::mutex> lock(_feederLock);
				_feederCondition.wait_for(lock, std::chrono::milliseconds(1));
			}
		}
		
		std::unique_lock<std::mutex> lock(_consumerLock);
		_consumerCondition.notify_all();
		
		tasks.clear();
		_feedLock.unlock();
	}
	
	void ThreadPool::Consumer()
	{
		Thread *thread = Thread::GetCurrentThread();
		AutoreleasePool *pool = new AutoreleasePool();
	
		size_t threadID = thread->GetObjectForKey<Number>(RNCSTR("__kRNThreadID"))->GetUint32Value();
		ThreadContext *local = _threadData[threadID];
		
		{
			std::unique_lock<std::mutex> lock(_syncLock);
			
			if(!_running.load())
				_syncpoint.wait(lock, [&] { return _running.load() == true; });
		}
		
		while(!thread->IsCancelled())
		{
			Task task;
			
			while(1)
			{
				bool result = local->hose.pop(task);
				if(!result)
					break;
				
				task.function();
					
				if(task.batch)
				{
					if((-- task.batch->_openTasks) == 0)
					{
						task.batch->_waitCondition.notify_all();
						task.batch->Release();
					}
				}
			}
			
			if(local->hose.was_empty())
			{
				std::unique_lock<std::mutex> lock(_consumerLock);
				
				_feederCondition.notify_one();
				_consumerCondition.wait(lock, [&]() { return (local->hose.was_empty() == false); });
			}
		}
		
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
		
		// This is a race condition, because we first check wether there are open tasks and THEN wait
		// In theory the lock should protect us from that, but the consumer don't lock it for performance reasons
		// So instead, the condition can timeout...
		
		std::unique_lock<std::mutex> lock(_lock);
		
		while(_openTasks.load() > 0)
			_waitCondition.wait_for(lock, std::chrono::nanoseconds(500), [&]{ return (_openTasks.load() == 0); });
		
		Release();
	}
}
