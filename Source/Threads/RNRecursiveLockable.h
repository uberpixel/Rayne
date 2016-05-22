//
//  RNRecursiveLockable.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RECURSIVELOCKABLE_H_
#define __RAYNE_RECURSIVELOCKABLE_H_

#ifdef RN_BUILD_LIBRARY
	#include <RayneConfig.h>
#else
	#include "../RayneConfig.h"
#endif

#include <atomic>
#include <thread>

namespace RN
{
	class RecursiveLockable
	{
	public:
		RecursiveLockable() :
			_threadRecursion(0)
		{
			_flag.store(0, std::memory_order_release);
		}

		void Lock()
		{
			uint8 value = 0;
			if(RN_EXPECT_TRUE(_flag.compare_exchange_weak(value, kLockFlagLocked, std::memory_order_acq_rel)))
			{
				_thread = std::this_thread::get_id();
				_threadRecursion = 1;

				return;
			}

			if(RN_EXPECT_TRUE(_thread == std::this_thread::get_id()))
			{
				_threadRecursion ++;
				return;
			}

			LockSlowPath();
		}

		bool TryLock()
		{
			while(1)
			{
				uint8 value = _flag.load(std::memory_order_acquire);
				if(value & kLockFlagLocked)
					return false;

				if(_flag.compare_exchange_weak(value, value | kLockFlagLocked))
				{
					_thread = std::this_thread::get_id();
					_threadRecursion = 1;
					return true;
				}
			}
		}

		void Unlock()
		{
			RN_ASSERT(IsAcquired(), "Lockable must be acquired in order to be released!");

			if((-- _threadRecursion) > 0)
				return;

			uint8 expected = kLockFlagLocked;

			if(RN_EXPECT_TRUE(_flag.compare_exchange_weak(expected, 0, std::memory_order_acq_rel)))
				return;

			UnlockSlowPath();
		}

		bool IsAcquired() const
		{
			return _flag.load(std::memory_order_acquire) & kLockFlagLocked;
		}

	protected:
		std::atomic<uint8> _flag;
		std::atomic<std::thread::id> _thread;
		size_t _threadRecursion;

	private:
		RNAPI void LockSlowPath();
		RNAPI void UnlockSlowPath();

		static constexpr uint8 kLockFlagLocked = (1 << 0);
		static constexpr uint8 kLockFlagParked = (1 << 1);
	};
}


#endif /* __RAYNE_RECURSIVELOCKABLE_H_ */
