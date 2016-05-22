//
//  RNLockable.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LOCKABLE_H_
#define __RAYNE_LOCKABLE_H_

#include "../Base/RNBase.h"

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
			uint8 value = 0;
			if(RN_EXPECT_TRUE(_flag.compare_exchange_weak(value, kLockFlagLocked, std::memory_order_acq_rel)))
				return;

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
					return true;
			}
		}

		void Unlock()
		{
			RN_ASSERT(IsLocked(), "Lockable must be acquired in order to be released!");

			uint8 expected = kLockFlagLocked;

			if(RN_EXPECT_TRUE(_flag.compare_exchange_weak(expected, 0, std::memory_order_acq_rel)))
				return;

			UnlockSlowPath();
		}

		bool IsLocked() const
		{
			return _flag.load(std::memory_order_acquire) & kLockFlagLocked;
		}

	protected:
		std::atomic<uint8> _flag;

	private:
		RNAPI void LockSlowPath();
		RNAPI void UnlockSlowPath();

		static constexpr uint8 kLockFlagLocked = (1 << 0);
		static constexpr uint8 kLockFlagParked = (1 << 1);
	};
}


#endif /* __RAYNE_LOCKABLE_H_ */
