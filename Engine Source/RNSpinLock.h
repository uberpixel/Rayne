//
//  RNSpinLock.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
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
			_lock = OS_SPINLOCK_INIT;
		}
		
		void Lock()
		{
			OSSpinLockLock(&_lock);
		}
		
		void Unlock()
		{
			OSSpinLockUnlock(&_lock);
		}
		
	private:
		OSSpinLock _lock;
	};
}

#endif /* __RAYNE_SPINLOCK_H__ */
