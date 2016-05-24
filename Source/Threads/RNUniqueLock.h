//
//  RNUniqueLock.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UNIQUELOCK_H_
#define __RAYNE_UNIQUELOCK_H_

#include "../Base/RNBase.h"

namespace RN
{
	enum class LockPolicy
	{
		Lock,
		TryLock,
		AdoptLock,
		DeferLock
	};

	template<class T>
	class UniqueLock
	{
	public:
		UniqueLock(T &lock, LockPolicy policy = LockPolicy::Lock) :
			_lock(lock),
			_ownsLock(false),
			_hasLock(true)
		{
			switch(policy)
			{
				case LockPolicy::Lock:
					Lock();
					break;
				case LockPolicy::TryLock:
					TryLock();
					break;
				case LockPolicy::AdoptLock:
					_ownsLock = lock.IsLocked();
					break;
				case LockPolicy::DeferLock:
					break;
			}
		}
		~UniqueLock()
		{
			if(_ownsLock)
				Unlock();
		}

		void Lock()
		{
			if(RN_EXPECT_FALSE(!_hasLock))
				return;

			_lock.Lock();
			_ownsLock = true;
		}
		void Unlock()
		{
			if(RN_EXPECT_FALSE(!_hasLock))
				return;

			_lock.Unlock();
			_ownsLock = false;
		}
		bool TryLock()
		{
			if(RN_EXPECT_FALSE(!_hasLock))
				return false;

			if(_lock.TryLock())
			{
				_ownsLock = true;
				return true;
			}

			return false;
		}

		bool OwnsLock() const
		{
			if(RN_EXPECT_FALSE(!_hasLock))
				return false;

			return _ownsLock;
		}

		void Release()
		{
			_hasLock = false;
		}

	private:
		bool _ownsLock;
		bool _hasLock;
		T &_lock;
	};
}


#endif /* __RAYNE_UNIQUELOCK_H_ */
