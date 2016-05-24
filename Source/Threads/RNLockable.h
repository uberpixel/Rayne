//
//  RNLockable.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LOCKABLE_H_
#define __RAYNE_LOCKABLE_H_

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
	class Lockable
	{
	public:
		Lockable()
		{
			_flag.store(0, std::memory_order_release);
		}

		void Lock()
		{
			if(RN_EXPECT_TRUE(__Private::CompareExchangeWeak<uint8>(_flag, 0, kLockFlagLocked, std::memory_order_acquire)))
			{
#if RN_BUILD_DEBUG
				_thread = std::this_thread::get_id();
#endif
				return;
			}

			LockSlowPath();
		}

		bool TryLock()
		{
			while(1)
			{
				uint8 value = _flag.load();
				if(value & kLockFlagLocked)
					return false;

				if(__Private::CompareExchangeWeak<uint8>(_flag, value, value | kLockFlagLocked))
				{
#if RN_BUILD_DEBUG
					_thread = std::this_thread::get_id();
#endif
					return true;
				}
			}
		}

		void Unlock()
		{
			RN_ASSERT(IsLocked(), "Lockable must be acquired in order to be released!");

#if RN_BUILD_DEBUG
			RN_ASSERT(_thread == std::this_thread::get_id(), "Lockable must be unlocked from the thread that locked it");
#endif

			if(RN_EXPECT_TRUE(__Private::CompareExchangeWeak<uint8>(_flag, kLockFlagLocked, 0, std::memory_order_release)))
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

#if RN_BUILD_DEBUG
		std::thread::id _thread;
#endif
	};
}


#endif /* __RAYNE_LOCKABLE_H_ */
