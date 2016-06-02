//
//  RNCondition.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CONDITION_H_
#define __RAYNE_CONDITION_H_

#ifdef RN_BUILD_LIBRARY
	#include <RayneConfig.h>
#else
	#include "../RayneConfig.h"
#endif

#include <atomic>

#include "RNFutex.h"

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

			bool result = __Private::Futex::Wait(&_hasWaiters, [this]() -> bool {
				_hasWaiters.store(true, std::memory_order_release);
				return true;
			}, [&lock]() {
				lock.Unlock();
			}, timeout);

			lock.Lock();
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

		template<class T, class Rep, class Period>
		bool WaitFor(T &lock, const std::chrono::duration<Rep, Period> &duration)
		{
			return WaitUntil(lock, Clock::now() + duration);
		}
		template<class T, class Rep, class Period, class Functor>
		bool WaitFor(T &lock, const std::chrono::duration<Rep, Period> &duration, const Functor &predicate)
		{
			return WaitUntil(lock, Clock::now() + duration, predicate);
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

			__Private::Futex::WakeOne(&_hasWaiters, [this](__Private::Futex::WakeResult result) {

				if(!(result & __Private::Futex::WakeResult::HasMoreThreads))
					_hasWaiters.store(false, std::memory_order_release);

			});
		}
		void NotifyAll()
		{
			if(!_hasWaiters.load(std::memory_order_acquire))
				return;

			_hasWaiters.store(std::memory_order_release);
			__Private::Futex::WakeAll(&_hasWaiters);
		}

	private:
		std::atomic<bool> _hasWaiters;
	};
}


#endif /* __RAYNE_CONDITION_H_ */
