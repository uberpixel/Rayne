//
//  RNSpinLock.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBase.h"
#include "RNThread.h"
#include "RNSpinLock.h"

namespace RN
{
	void RecursiveSpinLock::Lock()
	{
		Thread *thread = Thread::GetCurrentThread();
		
		do {
			while(_flag.test_and_set(std::memory_order_acquire))
			{}
			
			Thread *owner = _owner.load();
			
			if(!owner || owner == thread)
			{
				_owner.store(thread);
				_locks ++;
				
				_flag.clear(std::memory_order_release);
				break;
			}
			
			_flag.clear(std::memory_order_release);
		} while(1);
	}
	
	void RecursiveSpinLock::Unlock()
	{
		if((-- _locks) == 0)
			_owner.store(nullptr);
	}
	
	bool RecursiveSpinLock::TryLock()
	{
		Thread *thread = Thread::GetCurrentThread();
		
		while(_flag.test_and_set(std::memory_order_acquire))
		{}
		
		Thread *owner = _owner.load();
		
		if(!owner || owner == thread)
		{
			_owner.store(thread);
			_locks ++;
			
			_flag.clear(std::memory_order_release);
			return true;
		}
		
		_flag.clear(std::memory_order_release);
		return false;
	}
}
