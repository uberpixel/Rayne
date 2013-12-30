//
//  RNSemaphore.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
