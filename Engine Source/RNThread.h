//
//  RNThread.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_THREAD_H__
#define __RAYNE_THREAD_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Mutex;
	class Thread : public Object
	{
	public:
		typedef void (*ThreadEntry)(Thread *thread);
		
		Thread(ThreadEntry entry);
		virtual ~Thread();
		
		bool OnThread() const;
		void Detach();
		
		static void Join(Thread *other);
		static Thread *CurrentThread();
		
	private:
		static void *Entry(void *object);
		
		bool _detached;
		ThreadEntry _entry;
		Mutex *_mutex;
		
#if RN_PLATFORM_POSIX
		pthread_t _thread;
#endif
#if RN_PLATFORM_WINDOWS
		HANDLE _thread;
		DWORD _id;
#endif
	};
}

#endif /* __RAYNE_THREAD_H__ */
