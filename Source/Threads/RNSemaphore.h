//
//  RNSemaphore.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SEMAPHORE_H__
#define __RAYNE_SEMAPHORE_H__

#include "../Base/RNBase.h"

namespace RN
{
#if RN_PLATFORM_MAC_OS

	class Semaphore
	{
	public:
		Semaphore(size_t count)
		{
			semaphore_create(mach_task_self(), &_semaphore, SYNC_POLICY_FIFO, static_cast<int>(count));
		}

		~Semaphore()
		{
			semaphore_destroy(mach_task_self(), _semaphore);
		}

		void Wait()
		{
			semaphore_wait(_semaphore);
		}

		void Signal()
		{
			semaphore_signal(_semaphore);
		}

	private:
		Semaphore(const Semaphore &other) = delete;
		Semaphore &operator=(const Semaphore &other) = delete;

		semaphore_t _semaphore;
	};

#endif

#if RN_PLATFORM_WINDOWS

	class Semaphore
	{
	public:
		Semaphore(size_t count)
		{
			_semaphore = ::CreateSemaphore(NULL, static_cast<int>(count), MAXLONG, NULL);
		}
		~Semaphore()
		{
			::CloseHandle(_semaphore);
		}

		void Wait()
		{
			WaitForSingleObject(_semaphore, INFINITE);
		}
		void Signal()
		{
			ReleaseSemaphore(_semaphore, 1, NULL);
		}

	private:
		Semaphore(const Semaphore &other) = delete;
		Semaphore &operator =(const Semaphore &other) = delete;

		HANDLE _semaphore;
	};

#endif
}

#endif /* __RAYNE_SEMAPHORE_H__ */
