//
//  RNLock.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LOCK_H_
#define __RAYNE_LOCK_H_

#include "../Base/RNBase.h"

namespace RN
{
	class Lock
	{
	public:
		Lock()
		{
			_flag.store(0, std::memory_order_release);
		}

		void Acquire()
		{
			uint8 value = 0;
			if(RN_EXPECT_TRUE(_flag.compare_exchange_weak(value, kLockFlagIsAcquired, std::memory_order_acq_rel)))
				return;

			AcquireSlowPath();
		}

		bool TryAcquire()
		{
			while(1)
			{
				uint8 value = _flag.load(std::memory_order_acquire);
				if(value & kLockFlagIsAcquired)
					return false;

				if(_flag.compare_exchange_weak(value, value | kLockFlagIsAcquired))
					return true;
			}
		}

		void Release()
		{
			RN_ASSERT(IsAcquired(), "Lock must be acquired in order to be released!");

			uint8 expected = kLockFlagIsAcquired;

			if(RN_EXPECT_TRUE(_flag.compare_exchange_weak(expected, 0, std::memory_order_acq_rel)))
				return;

			ReleaseSlowPath();
		}

		bool IsAcquired() const
		{
			return _flag.load(std::memory_order_acquire) & kLockFlagIsAcquired;
		}

	protected:
		std::atomic<uint8> _flag;

	private:
		RNAPI void AcquireSlowPath();
		RNAPI void ReleaseSlowPath();

		static constexpr uint8 kLockFlagIsAcquired = (1 << 0);
		static constexpr uint8 kLockFlagIsParked = (1 << 1);
	};
}


#endif /* __RAYNE_LOCK_H_ */
