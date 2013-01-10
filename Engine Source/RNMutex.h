//
//  RNMutex.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MUTEX_H__
#define __RAYNE_MUTEX_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Mutex : public Object
	{
	public:
		Mutex();
		virtual ~Mutex();
		
		void Lock();
		bool TryLock();
		void Unlock();
		
	private:
#if RN_PLATFORM_POSIX
		pthread_mutex_t _mutex;
		pthread_mutexattr_t _attribute;
#endif
		
#if RN_PLATFORM_WINDOWS
		HANDLE _mutex;
#endif
	};
}

#endif /* __RAYNE_MUTEX_H__ */
