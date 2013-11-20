//
//  RNSpinLock.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SPINLOCK_H__
#define __RAYNE_SPINLOCK_H__

#include "RNDefines.h"

namespace RN
{
	class SpinLock
	{
	public:
		SpinLock() :
			_flag(ATOMIC_FLAG_INIT)
		{
		}
		
		void Lock()
		{
			while(_flag.test_and_set(std::memory_order_acquire))
			{}
		}
		
		void Unlock()
		{
			_flag.clear(std::memory_order_release);
		}
		
		bool TryLock()
		{
			return (!_flag.test_and_set(std::memory_order_acquire));
		}
		
	private:
		std::atomic_flag _flag;
	};
}

#endif /* __RAYNE_SPINLOCK_H__ */
