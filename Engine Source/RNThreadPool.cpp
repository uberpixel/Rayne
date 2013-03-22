//
//  RNThreadPool.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNThreadPool.h"

namespace RN
{
	ThreadCoordinator::ThreadCoordinator()
	{
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
