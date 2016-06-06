//
//  RNLockable.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Base/RNBase.h"
#include "RNLockable.h"
#include "RNFutex.h"

namespace RN
{
	void Lockable::LockSlowPath()
	{
		size_t spinCount = 0;
		constexpr size_t spinLimit = 40;

		while(1)
		{
			uint8 value = _flag.load(std::memory_order_acquire);

			if(!(value & kLockFlagLocked) && __Private::CompareExchangeWeak<uint8>(_flag, value, value | kLockFlagLocked))
				return;

			if(!(value & kLockFlagParked) && spinCount < spinLimit)
			{
				spinCount ++;
				std::this_thread::yield();

				continue;
			}

			if(!(value & kLockFlagParked) && !__Private::CompareExchangeWeak<uint8>(_flag, value, value | kLockFlagParked))
				continue;

			__Private::Futex::CompareAndWait(&_flag, kLockFlagParked | kLockFlagLocked);
		}
	}

	void Lockable::UnlockSlowPath()
	{
		while(1)
		{
			uint8 value = _flag.load(std::memory_order_acquire);

			RN_ASSERT(value == kLockFlagLocked || value == (kLockFlagLocked | kLockFlagParked), "UnlockSlowPath found flag in inconsistent state, value: %d", value);

			if(value == kLockFlagLocked)
			{
				if(!__Private::CompareExchangeWeak<uint8>(_flag, kLockFlagLocked, 0))
					continue;

				return;
			}

			RN_DEBUG_ASSERT(value == (kLockFlagLocked | kLockFlagParked), "UnlockSlowPath found flag in inconsistent state");

			__Private::Futex::WakeOne(&_flag, [this](__Private::Futex::WakeResult result) {

				RN_DEBUG_ASSERT(_flag.load() == (kLockFlagLocked | kLockFlagParked), "UnlockSlowPath found flag in inconsistent state");

				if(result & __Private::Futex::WakeResult::HasMoreThreads)
					_flag.store(kLockFlagParked, std::memory_order_release);
				else
					_flag.store(0, std::memory_order_release);

			});

			return;
		}
	}
}
