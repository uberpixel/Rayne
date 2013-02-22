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
	ThreadPool::ThreadPool()
	{
		_baseConcurrency = (machine_int)std::thread::hardware_concurrency();
		_consumedConcurrency = 0;
		
		_baseConcurrency = MAX(_baseConcurrency, 4);
	}
	
	void ThreadPool::ConsumeConcurrency()
	{
		_lock.Lock();
		_consumedConcurrency ++;
		_lock.Unlock();
	}
	
	void ThreadPool::RestoreConcurrency()
	{
		_lock.Lock();
		_consumedConcurrency --;
		_lock.Unlock();
	}
	
	machine_int ThreadPool::AvailableConcurrency()
	{
		_lock.Lock();
		machine_int concurrency = _baseConcurrency - _consumedConcurrency;
		_lock.Unlock();
		
		return concurrency;
	}
}
