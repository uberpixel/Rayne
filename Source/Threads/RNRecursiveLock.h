//
//  RNRecursiveLock.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RECURSIVELOCK_H_
#define __RAYNE_RECURSIVELOCK_H_

#include "../Base/RNBase.h"

namespace RN
{
	class RecursiveLock
	{
	public:
		RecursiveLock() :
			_threadRecursion(0)
		{
			_flag.store(0, std::memory_order_release);
		}

		void Acquire()
		{
			uint8 value = 0;
			if(RN_EXPECT_TRUE(_flag.compare_exchange_weak(value, kLockFlagIsAcquired, std::memory_order_acq_rel)))
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
				{
					_thread = std::this_thread::get_id();
					_threadRecursion = 1;
					return true;
				}
			}
		}

		void Release()
		{
			RN_ASSERT(IsAcquired(), "Lock must be acquired in order to be released!");

			if((-- _threadRecursion) > 0)
				return;

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
		std::atomic<std::thread::id> _thread;
		size_t _threadRecursion;

	private:
		RNAPI void AcquireSlowPath();
		RNAPI void ReleaseSlowPath();

		static constexpr uint8 kLockFlagIsAcquired = (1 << 0);
		static constexpr uint8 kLockFlagIsParked = (1 << 1);
	};
}


#endif /* __RAYNE_RECURSIVELOCK_H_ */
