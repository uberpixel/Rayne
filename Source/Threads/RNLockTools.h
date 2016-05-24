//
//  RNLockTools.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LOCKTOOLS_H_
#define __RAYNE_LOCKTOOLS_H_

#include <atomic>

namespace RN
{
	template<class T0, class T1>
	void Lock(T0 &_lock0, T1 &_lock1)
	{
		T0 *lock0 = &_lock0;
		T1 *lock1 = &_lock1;

		if(lock0 > lock1)
		{
			lock1->Lock();
			lock0->Lock();
		}
		else
		{
			lock0->Lock();
			lock1->Lock();
		}
	}

	namespace __Private
	{
		template<class T>
		bool CompareExchangeWeak(std::atomic<T> &atomic, T expected, T desired, std::memory_order order = std::memory_order_seq_cst)
		{
#if RN_PLATFORM_WINDOWS
			// Windows apparently does weird stuff if the order is not seq_cst
			order = std::memory_order_seq_cst;
#endif

			T expectedAndActual = expected;
			return atomic.compare_exchange_weak(expectedAndActual, desired, order);
		}
	}
}


#endif /* __RAYNE_LOCKTOOLS_H_ */
