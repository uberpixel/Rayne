//
//  RNThreadPool.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		
		typedef uint32 BatchID;
		typedef std::chrono::time_point<std::chrono::steady_clock> ClockType;
		
		ThreadPool(PoolType type, machine_uint maxThreads=0)
		{
			_type         = type;
			_resigned     = 0;
			_currentID    = 0;
			_currentBatch = 0;
			
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
			
			_lock.Lock();
			_workQueue.clear();
			_lock.Unlock();
			
			_waitCondition.notify_all(); // Notify all sleeping threads
			
			std::unique_lock<std::mutex> lock(_tearDownMutex);
			_tearDownCondition.wait(lock, [&]() { return (_resigned == _threads.Count()); } );
			
			// Clear all batches
			std::vector<Batch *> batches;
			batches.insert(batches.end(), _activeBatches.begin(), _activeBatches.end());
			batches.insert(batches.end(), _resignedBatches.begin(), _resignedBatches.end());
			
			for(Batch *batch : batches)
			{
				delete batch;
			}
		}
		
		
		template<typename F>
		void AddTask(F&& f, ClockType timeout = ClockType::max())
		{
			AddTaskWithPredicate(f, std::function<bool ()>(), timeout);
		}
		
		template<typename F>
		void AddTaskWithPredicate(F&& f, const std::function<bool ()>& predicate, ClockType timeout = ClockType::max())
		{
			Task temp;
			temp.task      = std::move(f);
			temp.predicate = predicate;
			temp.timeout   = timeout;
			temp.batch     = _currentBatch;
			
			if(!_currentBatch)
			{
				_lock.Lock();
				_workQueue.push_back(std::move(temp));
				_lock.Unlock();
				
				_waitCondition.notify_one();
			}
			else
			{
				_currentBatch->queuedTasks.push_back(std::move(temp));
			}
		}
		
		
		template<typename F>
		std::future<typename std::result_of<F()>::type> AddFutureTask(F&& f, ClockType timeout = ClockType::max())
		{
			return AddFutureTaskWithPredicate(f, std::function<bool ()>(), timeout);
		}
		
		template<typename F>
		std::future<typename std::result_of<F()>::type> AddFutureTaskWithPredicate(F&& f, const std::function<bool ()>& predicate, ClockType timeout = ClockType::max())
		{
			typedef typename std::result_of<F()>::type resultType;
			
			std::packaged_task<resultType ()> task(std::move(f));
			std::future<resultType> result(task.get_future());
			
			Task temp;
			temp.task      = std::move(task);
			temp.predicate = predicate;
			temp.timeout   = timeout;
			temp.batch     = _currentBatch;
			
			if(!_currentBatch)
			{
				_lock.Lock();
				_workQueue.push_back(std::move(temp));
				_lock.Unlock();
				
				_waitCondition.notify_one();
			}
			else
			{
				_currentBatch->queuedTasks.push_back(std::move(temp));
			}
			
			return result;
		}
		
		
		
		BatchID BeginTaskBatch()
		{
			RN_ASSERT0(_currentBatch == 0);
			
			if(_resignedBatches.size() > 0)
			{
				_currentBatch = _resignedBatches.back();
				_resignedBatches.pop_back();
			}
			else
			{
				_currentBatch = new Batch;
			}
			
			_currentBatch->batch = (++ _currentID);
			_currentBatch->tasks.store(0);
			
			return _currentBatch->batch;
		}
		
		void CommitTaskBatch(bool waitForCompletion=false)
		{
			RN_ASSERT0(_currentBatch);
			Batch *batch = _currentBatch;
			
			_activeBatches.push_back(_currentBatch);
			_currentBatch = 0;
			
			_lock.Lock();
			
			batch->tasks.store((uint32)batch->queuedTasks.size());
			
			std::move(batch->queuedTasks.begin(), batch->queuedTasks.end(), std::back_inserter(_workQueue));
			batch->queuedTasks.clear();
			   
			_lock.Unlock();
			_waitCondition.notify_all();
			
			if(waitForCompletion)
			{
				std::unique_lock<std::mutex> lock(batch->mutex);
				batch->condition.wait(lock, [&]() { return (batch->tasks.load() == 0); });
			}
		}
		
		void WaitForBatch(BatchID batchID)
		{
			Batch *batch = 0;
			
			_lock.Lock();
			
			for(Batch *temp : _activeBatches)
			{
				if(temp->batch == batchID)
				{
					batch = temp;
					break;
				}
			}
			
			_lock.Unlock();
			
			if(!batch)
				return;
			
			std::unique_lock<std::mutex> lock(batch->mutex);
			batch->condition.wait(lock, [&]() { return (batch->tasks.load() == 0); });
		}
		
	private:
		struct Batch;
		class Task
		{
		public:
			std::function<bool ()> predicate;
			ClockType timeout;
			
			Function task;
			Batch *batch;
		};
		
		struct Batch
		{
			BatchID batch;
			std::atomic<uint32> tasks;
			std::deque<Task> queuedTasks;
			
			std::mutex mutex;
			std::condition_variable condition;
		};
		
		
		
		Thread *CreateThread()
		{
			Thread *thread = new Thread(std::bind(&ThreadPool::Consumer, this), false);
			_threads.AddObject(thread);
			
			return thread->Autorelease();
		}
		
		void Consumer()
		{
			Thread *thread = Thread::CurrentThread();
			Context *context = new Context(Kernel::SharedInstance()->Context());
			
			context->MakeActiveContext();
			
			std::deque<Task> localQueue;
			std::deque<Task> backfeed;
			
			while(!thread->IsCancelled())
			{
				if(localQueue.size() == 0)
				{
					_lock.Lock();
					
					// Check if an actively running batch has finished
					for(auto i=_activeBatches.begin(); i!=_activeBatches.end();)
					{
						Batch *batch = *i;
						
						if(batch->tasks.load() == 0)
						{
							batch->condition.notify_all();
							_resignedBatches.push_back(batch);
							
							i = _activeBatches.erase(i);
							continue;
						}
						
						i ++;
					}
					
					if(backfeed.size() > 0)
					{
						if(_workQueue.size() == 0)
						{
							std::move(backfeed.begin(), backfeed.end(), std::back_inserter(localQueue));
							backfeed.clear();
							
							_lock.Unlock();
							continue;
						}
						else
						{
							std::move(backfeed.begin(), backfeed.end(), std::back_inserter(_workQueue));
							backfeed.clear();
						}
					}
					
					if(_workQueue.size() == 0)
					{
						// TODO: Check if we can steal tasks from other threads
						_lock.Unlock();
						
						std::unique_lock<std::mutex> lock(_waitMutex);
						_waitCondition.wait(lock); // Spurious wake ups are okay, the cost of a predicate is higher than looping again.
						continue;
					}
					
					machine_uint moveLocal = MIN(kRNThreadPoolLocalQueueMaxSize, _workQueue.size());
					std::deque<Task>::iterator last = _workQueue.begin();
					std::advance(last, moveLocal);
					
					std::move(_workQueue.begin(), last, std::back_inserter(localQueue));
					_workQueue.erase(_workQueue.begin(), last);

					_lock.Unlock();
				}
				
				ClockType now = std::chrono::steady_clock::now();
				Task task = std::move(localQueue.front());
				localQueue.pop_front();
				
				if(task.timeout < now)
				{
					if(task.batch)
						task.batch->tasks --;

					continue;
				}
				
				if(task.predicate && !task.predicate())
				{
					backfeed.push_back(std::move(task));
					continue;
				}
				
				task.task();
				
				if(task.batch)
					task.batch->tasks --;
			}
			
			context->Release();
			
			_resigned ++;
			_tearDownCondition.notify_all();
		}
		
		SpinLock _lock;
		PoolType _type;
		
		BatchID _currentID;
		Batch  *_currentBatch;
		
		Array<Thread> _threads;
		machine_uint _resigned;
		
		std::deque<Task> _workQueue;
		
		std::vector<Batch *> _activeBatches;
		std::vector<Batch *> _resignedBatches;
	
		std::mutex _tearDownMutex;
		std::mutex _waitMutex;
		std::condition_variable _tearDownCondition;
		std::condition_variable _waitCondition;
	};
}

#endif /* __RAYNE_THREADPOOL_H__ */
