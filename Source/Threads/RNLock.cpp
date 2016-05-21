//
//  RNLock.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLock.h"
#include "RNParkingLot.h"

namespace RN
{
	void Lock::AcquireSlowPath()
	{
		size_t spinCount = 0;
		constexpr size_t spinLimit = 40;

		while(1)
		{
			uint8 value = _flag.load(std::memory_order_acquire);


			if(!(value & kLockFlagIsAcquired) && _flag.compare_exchange_weak(value, value | kLockFlagIsAcquired, std::memory_order_release))
				return;

			if(!(value & kLockFlagIsParked) && spinCount < spinLimit)
			{
				spinCount ++;
				std::this_thread::yield();

				continue;
			}

			if(!(value & kLockFlagIsParked) && !_flag.compare_exchange_weak(value, value | kLockFlagIsParked, std::memory_order_release))
				continue;

			ParkingLot::CompareAndPark(&_flag, kLockFlagIsParked | kLockFlagIsAcquired);
		}
	}

	void Lock::ReleaseSlowPath()
	{
		while(1)
		{
			uint8 value = _flag.load(std::memory_order_acquire);

			if(value == kLockFlagIsAcquired)
			{
				uint8 expected = kLockFlagIsAcquired;

				if(!_flag.compare_exchange_weak(expected, 0, std::memory_order_release))
					continue;

				return;
			}

			ParkingLot::UnparkThread(&_flag, [this](ParkingLot::UnparkResult result) {

				if(result & ParkingLot::UnparkResult::HasMoreThreads)
					_flag.store(kLockFlagIsParked, std::memory_order_release);
				else
					_flag.store(0, std::memory_order_release);

			});
		}
	}
}
