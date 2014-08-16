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
			auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::nanoseconds(850);
			
			do {
				for(size_t i = 0; i < std::numeric_limits<int16>::max(); i ++)
				{
					bool result = _spinLock.test_and_set(std::memory_order_acquire);
					if(!result) // We got the lock
					{
						acquired = true;
						break;
					}
				}
			} while(!acquired && std::chrono::high_resolution_clock::now() < timeout);
			
			if(!acquired)
			{
				_waiting ++;
				
				// To avoid race conditions, check if the predicate became true in the meantime
				if(_spinLock.test_and_set(std::memory_order_acquire))
				{
					_waiting --;
					return;
				}
				
				std::unique_lock<std::mutex> lock(_lock);
				_signal.wait(lock, [&] { return (_spinLock.test_and_set(std::memory_order_acquire) == false); });
				_waiting --;
			}
		}
		
		bool TryLock()
		{
			bool result = _spinLock.test_and_set(std::memory_order_acquire);
			return (result == false);
		}
		
		void Unlock()
		{
			_spinLock.clear(std::memory_order_release);
			
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
