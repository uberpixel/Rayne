//
//  RNAdaptiveLock.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ADAPTIVELOCK_H_
#define __RAYNE_ADAPTIVELOCK_H_

#include "../Base/RNBase.h"

#if RN_PLATFORM_INTEL
#if RN_PLATFORM_WINDOWS
		#define RNHardwarePause() YieldProcessor()
	#else
		#define RNHardwarePause() __asm__ volatile("pause")
	#endif
#endif
#if RN_PLATFORM_ARM
#define RNHardwarePause() __asm__ volatile("yield")
#endif

#define RNConditionalSpin(e, count) \
	({ \
		bool result = false; \
		for(size_t i = 0; i < count; i ++) \
		{ \
			if((e)) \
			{ \
				result = true; \
				break; \
			} \
			RNHardwarePause(); \
		} \
		result; \
	})

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
			if(!_spinLock.test_and_set(std::memory_order_acquire))
				return; // Fast path for zero contention cases

			bool acquired = false;
			auto timeout = Clock::now() + std::chrono::nanoseconds(850);

			do {

				if(RNConditionalSpin(_spinLock.test_and_set(std::memory_order_acquire), 10535U))
				{
					acquired = true;
					break;
				}

			} while(Clock::now() < timeout);

			if(!acquired)
			{
				_waiting ++;
				
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


#endif /* __RAYNE_ADAPTIVELOCK_H_ */
