//
//  RNPThreadPark.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_THREADPARK_H_
#define __RAYNE_THREADPARK_H_

#include <RayneConfig.h>

#include "../Base/RNOptions.h"
#include <atomic>
#include <functional>

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
				auto validate = [&validation]() -> bool {
					return validation();
				};
				auto prepare = [&beforeSleep]() {
					beforeSleep();
				};
				return __WaitBorrowed(address, &validate, [](void *context) -> bool { return (*static_cast<decltype(validate) *>(context))(); }, &prepare, [](void *context) { (*static_cast<decltype(prepare) *>(context))(); }, timeout);
			}

			template<class Validation>
			static bool Wait(const void *address, Validation &&validation)
			{
				return Wait(address, std::forward<Validation>(validation), []() {}, Clock::time_point::max());
			}

			static bool Wait(const void *address)
			{
				return Wait(address, []() -> bool { return true; });
			}

			template<class T, class U>
			static bool CompareAndWait(const std::atomic<T> *address, U expected)
			{
				return Wait(address, [address, expected]() -> bool {
					U value = address->load();
					return (value == expected);
				});
			}

			template<class Callback>
			static void WakeOne(const void *address, Callback &&callback)
			{
				auto wake = [&callback](WakeResult result) {
					callback(result);
				};
				__WakeOneBorrowed(address, &wake, [](void *context, WakeResult result) {
					(*static_cast<decltype(wake) *>(context))(result);
				});
			}

			RNAPI static WakeResult WakeOne(const void *address);
			RNAPI static void WakeOne(const void *address, std::function<void(WakeResult)> callback);
			RNAPI static void WakeAll(const void *address);

		private:
			RNAPI static bool __WaitBorrowed(const void *address, void *validationContext, bool (*validation)(void *), void *beforeSleepContext, void (*beforeSleep)(void *), Clock::time_point timeout);
			RNAPI static void __WakeOneBorrowed(const void *address, void *context, void (*callback)(void *, WakeResult));
			RNAPI static bool __WaitConditionally(const void *address, std::function<bool()> validation, std::function<void()> beforeSleep, Clock::time_point timeout);
		};
	} // namespace __Private
} // namespace RN


#endif /* __RAYNE_THREADPARK_H_ */
