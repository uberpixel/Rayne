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

#include "RNLockTools.h"

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
			if(RN_EXPECT_TRUE(__Private::CompareExchangeWeak<uint8>(_flag, 0, kLockFlagLocked)))
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

				if(__Private::CompareExchangeWeak<uint8>(_flag, value, value | kLockFlagLocked))
				{
					_thread = std::this_thread::get_id();
					_threadRecursion = 1;
					return true;
				}
			}
		}

		void Unlock()
		{
			RN_DEBUG_ASSERT(IsLocked(), "RecursiveLockable must be acquired in order to be released!");
			RN_DEBUG_ASSERT(_thread.load(std::memory_order_acquire) == std::this_thread::get_id(), "RecursiveLockable must be unlocked from the thread that locked it");

			if((-- _threadRecursion) > 0)
				return;

			if(RN_EXPECT_TRUE(__Private::CompareExchangeWeak<uint8>(_flag, kLockFlagLocked, 0)))
				return;

			UnlockSlowPath();
		}

		bool IsLocked() const
		{
			return _flag.load(std::memory_order_acquire) & kLockFlagLocked;
		}

	private:
		RNAPI void LockSlowPath();
		RNAPI void UnlockSlowPath();

		static constexpr uint8 kLockFlagLocked = (1 << 0);
		static constexpr uint8 kLockFlagParked = (1 << 1);

		std::atomic<uint8> _flag;
		std::atomic<std::thread::id> _thread;
		size_t _threadRecursion;
	};
}


#endif /* __RAYNE_RECURSIVELOCKABLE_H_ */
