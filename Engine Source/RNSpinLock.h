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

typedef uint8 RNPrimitiveSpinLock;

extern "C"
{
	void RNPrimitiveSpinLockLock(RNPrimitiveSpinLock *lock);
	void RNPrimitiveSpinLockUnlock(RNPrimitiveSpinLock *lock);
	bool RNPrimitiveSpinLockTryLock(RNPrimitiveSpinLock *lock);
}

namespace RN
{
	class SpinLock
	{
	public:
		SpinLock()
		{
			_lock = 0;
		}
		
		void Lock()
		{
			RNPrimitiveSpinLockLock(&_lock);
		}
		
		void Unlock()
		{
			RNPrimitiveSpinLockUnlock(&_lock);
		}
		
		bool TryLock()
		{
			return RNPrimitiveSpinLockTryLock(&_lock);
		}
		
	private:
		RNPrimitiveSpinLock _lock;
	};
}

#endif /* __RAYNE_SPINLOCK_H__ */
