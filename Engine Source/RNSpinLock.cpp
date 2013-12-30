//
//  RNSpinLock.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBase.h"
#include "RNThread.h"
#include "RNSpinLock.h"

namespace RN
{
	void RecursiveSpinLock::Lock()
	{
		auto thread = std::this_thread::get_id();
		
		do {
			while(_flag.test_and_set(std::memory_order_acquire))
			{}
			
			auto locks = _locks.load();
			if(locks == 0 || _owner == thread)
			{
				_locks ++;
				_owner = thread;
				
				_flag.clear(std::memory_order_release);
				break;
			}
			
			_flag.clear(std::memory_order_release);
		} while(1);
	}
	
	void RecursiveSpinLock::Unlock()
	{
		_locks --;
	}
	
	bool RecursiveSpinLock::TryLock()
	{
		auto thread = std::this_thread::get_id();
		
		if(_flag.test_and_set(std::memory_order_acquire))
			return false;
		
		auto locks = _locks.load();
		if(locks == 0 || _owner == thread)
		{
			_locks ++;
			_owner = thread;
			
			_flag.clear(std::memory_order_release);
			return true;
		}
		
		_flag.clear(std::memory_order_release);
		return false;
	}
}
