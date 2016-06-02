//
//  RNLockGuard.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LOCKGUARD_H__
#define __RAYNE_LOCKGUARD_H__

namespace RN
{
	template<class T>
	class LockGuard
	{
	public:
		LockGuard(T &lock) :
			_lock(lock)
		{
			_lock.Lock();
		}
		~LockGuard()
		{
			_lock.Unlock();
		}

	private:
		T &_lock;
	};
}

#endif
