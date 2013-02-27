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
		_baseConcurrency = (machine_int)std::thread::hardware_concurrency();
		_consumedConcurrency = 0;
		
		_baseConcurrency = MAX(_baseConcurrency, 4);
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
}
