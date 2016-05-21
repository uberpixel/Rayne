//
//  RNCondition.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CONDITION_H_
#define __RAYNE_CONDITION_H_

#include "../Base/RNBase.h"
#import "RNParkingLot.h"

namespace RN
{
	class Condition
	{
	public:
		Condition() :
			_hasWaiters(0)
		{}

		template<class T>
		bool WaitUntil(T &lock, Clock::time_point timeout)
		{
			if(timeout < Clock::now())
				return false;

			bool result = ParkingLot::Park(&_hasWaiters, [this]() -> bool {
				_hasWaiters.store(true, std::memory_order_release);
				return true;
			}, [&lock]() { lock.Release(); }, timeout);

			lock.Acquire();
			return result;
		}
		template<class T, class Functor>
		bool WaitUntil(T &lock, Clock::time_point timeout, const Functor &predicate)
		{
			while(!predicate())
			{
				if(!WaitUntil(lock, timeout))
					return predicate();
			}

			return true;
		}

		template<class T>
		void Wait(T &lock)
		{
			WaitUntil(lock, Clock::time_point::max());
		}
		template<class T, class Functor>
		void Wait(T &lock, const Functor &predicate)
		{
			WaitUntil(lock, Clock::time_point::max(), predicate);
		}

		void NotifyOne()
		{
			if(!_hasWaiters.load(std::memory_order_acquire))
				return;

			ParkingLot::UnparkThread(&_hasWaiters, [this](ParkingLot::UnparkResult result) {

				if(!(result & ParkingLot::UnparkResult::HasMoreThreads))
					_hasWaiters.store(0, std::memory_order_release);

			});
		}
		void NotifyAll()
		{
			if(!_hasWaiters.load(std::memory_order_acquire))
				return;

			_hasWaiters.store(std::memory_order_release);
			ParkingLot::UnparkAllThreads(&_hasWaiters);
		}

	private:
		std::atomic<bool> _hasWaiters;
	};
}


#endif /* __RAYNE_CONDITION_H_ */
