//
//  RNMutex.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMutex.h"

namespace RN
{
	RNDeclareMeta(Mutex)
	
	Mutex::Mutex()
	{
#if RN_PLATFORM_POSIX
		int error = 0;
		
		error = pthread_mutexattr_init(&_attribute);
		RN_ASSERT0(error == 0);
		
		pthread_mutexattr_settype(&_attribute, PTHREAD_MUTEX_NORMAL);
		
		error = pthread_mutex_init(&_mutex, &_attribute);
		RN_ASSERT0(error == 0);
#endif
		
#if RN_PLATFORM_WINDOWS
		_mutex = CreateMutex(NULL, false, NULL);
		RN_ASSERT0(_mutex);
#endif
	}
	
	Mutex::~Mutex()
	{
#if RN_PLATFORM_POSIX
		pthread_mutex_destroy(&_mutex);
		pthread_mutexattr_destroy(&_attribute);
#endif
		
#if RN_PLATFORM_WINDOWS
		CloseHandle(_mutex);
#endif
	}
	
	void Mutex::Lock()
	{
#if RN_PLATFORM_POSIX
		pthread_mutex_lock(&_mutex);
#endif
		
#if RN_PLATFORM_WINDOWS
		WaitForSingleObject(_mutex, INFINITE);
#endif
	}
	
	bool Mutex::TryLock()
	{
#if RN_PLATFORM_POSIX
		int result = pthread_mutex_trylock(&_mutex);
		return (result != EBUSY);
#endif
		
#if RN_PLATFORM_WINDOWS
		DWORD result = WaitForSingleObject(_mutex, 2);
		return !(result == WAIT_TIMEOUT);
#endif
	}
	
	void Mutex::Unlock()
	{
#if RN_PLATFORM_POSIX
		pthread_mutex_unlock(&_mutex);
#endif
		
#if RN_PLATFORM_WINDOWS
		ReleaseMutex(_mutex);
#endif
	}
}
