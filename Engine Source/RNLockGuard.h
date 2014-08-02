//
//  RNLockGuard.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LOCKGUARD_H__
#define __RAYNE_LOCKGUARD_H__

namespace RN
{
	template<class T, bool ptr>
	class __LockGuardCore
	{};
	
	template<class T>
	class __LockGuardCore<T, true>
	{
	public:
		__LockGuardCore(T lock) :
			_lock(lock)
		{
			Lock();
		}
		
		~__LockGuardCore()
		{
			if(_ownsLock)
				Unlock();
		}
		
		
		void Lock()
		{
			_lock->Lock();
			_ownsLock = true;
		}
		
		void Unlock()
		{
			_lock->Unlock();
			_ownsLock = false;
		}
		
	private:
		bool _ownsLock;
		T _lock;
	};
	
	template<class T>
	class __LockGuardCore<T, false>
	{
	public:
		__LockGuardCore(T &lock) :
			_lock(lock)
		{
			Lock();
		}
		
		~__LockGuardCore()
		{
			if(_ownsLock)
				Unlock();
		}
		
		
		void Lock()
		{
			_lock.Lock();
			_ownsLock = true;
		}
		
		void Unlock()
		{
			_lock.Unlock();
			_ownsLock = false;
		}
		
	private:
		bool _ownsLock;
		T &_lock;
	};
	
	template<class T>
	using LockGuard = __LockGuardCore<T, std::is_pointer<T>::value>;
}

#endif
