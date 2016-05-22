//
//  RNSTL.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STL_H__
#define __RAYNE_STL_H__

namespace RN
{
	namespace stl
	{
		template<class T>
		class lockable_shim
		{
		public:
			lockable_shim(T &lock) :
				_lock(lock)
			{}
		
			bool try_lock()
			{
				return _lock.TryLock();
			}
			
			void lock()
			{
				_lock.Lock();
			}
			
			void unlock()
			{
				_lock.Unlock();
			}
			
		private:
			T &_lock;
		};
	}
}

#endif
