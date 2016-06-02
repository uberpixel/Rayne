//
//  RNLockWrapper.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LOCKWRAPPER_H_
#define __RAYNE_LOCKWRAPPER_H_

namespace RN
{
	template<class T>
	class LockWrapper
	{
	public:
		LockWrapper(T &lock) :
			_lock(lock)
		{}

		void Lock()
		{
			_lock.Lock();
		}
		void Unlock()
		{
			_lock.Unlock();
		}
		bool TryLock()
		{
			return _lock.TryLock();
		}
		bool IsLocked() const
		{
			return _lock.IsLocked();
		}

	private:
		T &_lock;
	};

	template<class T>
	class LockWrapper<T *>
	{
	public:
		LockWrapper(T *lock) :
			_lock(lock)
		{}

		void Lock()
		{
			_lock->Lock();
		}
		void Unlock()
		{
			_lock->Unlock();
		}
		bool TryLock()
		{
			return _lock->TryLock();
		}
		bool IsLocked() const
		{
			return _lock->IsLocked();
		}

	private:
		T *_lock;
	};
}


#endif /* __RAYNE_LOCKWRAPPER_H_ */
