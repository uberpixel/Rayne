//
//  RNSemaphore.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SEMAPHORE_H__
#define __RAYNE_SEMAPHORE_H__

#include "RNBase.h"

namespace RN
{
	class Semaphore
	{
	public:
		Semaphore(size_t count) :
			_count(count),
			_initial(count)
		{}
		
		~Semaphore()
		{
			RN_ASSERT(_count >= _initial, "Semaphore destructor called while semaphore is still in use.");
			
#if RN_PLATFORM_MAC_OS
			// Works around a bug in OS X, which sometimes crashes mutexes and condition variables
			// https://devforums.apple.com/thread/220316?tstart=0
			
			struct timespec time { .tv_sec = 0, .tv_nsec = 1 };
			pthread_cond_timedwait_relative_np(_condition.native_handle(), _mutex.native_handle(), &time);
#endif
		}
		
		void Signal()
		{
			std::lock_guard<std::mutex> lock(_mutex);
			
			_count ++;
			_condition.notify_one();
		}
		
		void Wait()
		{
			std::unique_lock<std::mutex> lock(_mutex);
			
			while(_count == 0)
				_condition.wait(lock);
			
			_count --;
		}
		
		bool TryWait()
		{
			std::lock_guard<std::mutex> lock(_mutex);
			if(_count)
			{
				_count --;
				return true;
			}
			
			return false;
		}
		
	private:
		size_t _count;
		size_t _initial;
		
		std::condition_variable _condition;
		std::mutex _mutex;
	};
}

#endif /* __RAYNE_SEMAPHORE_H__ */
