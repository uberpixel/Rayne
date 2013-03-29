//
//  RNThreadPool.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNThreadPool.h"

namespace RN
{
	ThreadCoordinator::ThreadCoordinator()
	{
		// Sid:
		// Even on non multicore systems we have a base concurrency of at least two threads
		// This means that even on single core CPUs we spawn up to three threads in the base
		// implementation using a thread pool with threads equal to the amount of concurrency available.
		// I'm not sure if that's a good idea, and I lack the hardware to test it...
		_baseConcurrency = MAX((machine_int)std::thread::hardware_concurrency(), 2);
		_consumedConcurrency = 0;
	}
	
	void ThreadCoordinator::ConsumeConcurrency()
	{
		_lock.Lock();
		_consumedConcurrency ++;
		_lock.Unlock();
	}
	
	void ThreadCoordinator::RestoreConcurrency()
	{
		_lock.Lock();
		_consumedConcurrency --;
		_lock.Unlock();
	}
	
	machine_int ThreadCoordinator::AvailableConcurrency()
	{
		_lock.Lock();
		machine_int concurrency = _baseConcurrency - _consumedConcurrency;
		_lock.Unlock();
		
		return concurrency;
	}
	
	ThreadPool *ThreadCoordinator::GlobalPool()
	{
		static std::once_flag once;
		std::call_once(once, [this]() {
			_globalPool = new ThreadPool(ThreadPool::PoolTypeConcurrent);
		});
		
		return _globalPool;
	}
}
