//
//  RNSpinLock.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SPINLOCK_H__
#define __RAYNE_SPINLOCK_H__

#include "RNBase.h"

namespace RN
{
	class SpinLock
	{
	public:
		SpinLock()
		{
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS
			_lock = OS_SPINLOCK_INIT;
#endif
		}
		
		void Lock()
		{
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS
			OSSpinLockLock(&_lock);
#endif
			
#if RN_PLATFORM_WINDOWS
			_lock.lock();
#endif
		}
		
		void Unlock()
		{
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS
			OSSpinLockUnlock(&_lock);
#endif
			
#if RN_PLATFORM_WINDOWS
			_lock.unlock();
#endif
		}
		
		bool TryLock()
		{
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS
			return OSSpinLockTry(&_lock);
#endif
			
#if RN_PLATFORM_WINDOWS
			return _lock.try_lock();
#endif
		}
		
	private:
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS
		OSSpinLock _lock;
#endif
#if RN_PLATFORM_WINDOWS
		std::mutex _lock;
#endif
	};
}

#endif /* __RAYNE_SPINLOCK_H__ */
