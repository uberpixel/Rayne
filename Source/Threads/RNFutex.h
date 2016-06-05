//
//  RNPThreadPark.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_THREADPARK_H_
#define __RAYNE_THREADPARK_H_

#ifdef RN_BUILD_LIBRARY
	#include <RayneConfig.h>
#else
	#include "../RayneConfig.h"
#endif

#include <atomic>
#include <functional>
#include "../Base/RNOptions.h"

namespace RN
{
	namespace __Private
	{
		class Futex
		{
		public:
			RN_OPTIONS(WakeResult, uint32,
				WokeUpThread = (1 << 0),
				HasMoreThreads = (1 << 1));

			Futex() = delete;
			Futex(const Futex &) = delete;

			template<class Validation, class BeforeSleep>
			static bool Wait(const void *address, Validation &&validation, BeforeSleep &&beforeSleep, Clock::time_point timeout)
			{
				return __WaitConditionally(address, std::function<bool()>(std::forward<Validation>(validation)), std::function<void()>(std::forward<BeforeSleep>(beforeSleep)), timeout);
			}

			template<class Validation>
			static bool Wait(const void *address, Validation &&validation)
			{
				return __WaitConditionally(address, std::function<bool()>(std::forward<Validation>(validation)), []() {}, Clock::time_point::max());
			}

			static bool Wait(const void *address)
			{
				return __WaitConditionally(address, []() -> bool {
					return true;
				}, []() {}, Clock::time_point::max());
			}

			template<class T, class U>
			static bool CompareAndWait(const std::atomic<T> *address, U expected)
			{
				return Wait(address, [address, expected]() -> bool {

					U value = address->load();
					return (value == expected);

				});
			}

			RNAPI static WakeResult WakeOne(const void *address);
			RNAPI static void WakeOne(const void *address, std::function<void(WakeResult)> callback);
			RNAPI static void WakeAll(const void *address);

		private:
			RNAPI static bool __WaitConditionally(const void *address, std::function<bool()> validation, std::function<void()> beforeSleep, Clock::time_point timeout);
		};
	}
}


#endif /* __RAYNE_THREADPARK_H_ */
