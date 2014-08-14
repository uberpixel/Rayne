//
//  RNAdaptiveLock.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ADAPTIVELOCK_H__
#define __RAYNE_ADAPTIVELOCK_H__

#include "RNBase.h"

namespace RN
{
	class AdaptiveLock
	{
	public:
		AdaptiveLock() :
			_waiting(0)
		{
			_spinLock.clear();
		}
		
		void Lock()
		{
			bool acquired = false;
			
			_waiting ++;
			
			for(size_t i = 0; i < INT16_MAX; i ++)
			{
				bool result = _spinLock.test_and_set(std::memory_order_acquire);
				if(!result) // We got the lock
				{
					acquired = true;
					break;
				}
			}
			
			if(!acquired)
			{
				std::unique_lock<std::mutex> lock(_lock);
				_signal.wait(lock, [&] { return (_spinLock.test_and_set(std::memory_order_acquire) == false); });
			}
			
			_waiting --;
		}
		
		bool TryLock()
		{
			bool result = _spinLock.test_and_set(std::memory_order_acquire);
			return (result == false);
		}
		
		void Unlock()
		{
			_spinLock.clear();
			
			if(_waiting > 0)
				_signal.notify_one();
		}
		
	private:
		std::mutex _lock;
		std::condition_variable _signal;
		std::atomic<uint8> _waiting;
		
		std::atomic_flag _spinLock;
	};
}

#endif /* __RAYNE_ADAPTIVELOCK_H__ */
