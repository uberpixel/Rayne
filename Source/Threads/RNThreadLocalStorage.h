//
//  RNThreadLocalStorage.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_THREADLOCALSTORAGE_H__
#define __RAYNE_THREADLOCALSTORAGE_H__

#include "../Base/RNBase.h"
#include "RNSpinLock.h"

namespace RN
{
	namespace stl
	{
		template<class T>
		class thread_local_storage
		{
		public:
			T &get()
			{
				_lock.Lock();
				T &value = _storage[std::this_thread::get_id()];
				_lock.Unlock();
				
				return value;
			}
			
			
			void clear()
			{
				_lock.Lock();
				_storage.clear();
				_lock.Unlock();
			}
			void clear_local()
			{
				_lock.Lock();
				_storage.erase(std::this_thread::get_id());
				_lock.Unlock();
			}
			
			
			std::vector<T> values() const
			{
				std::vector<T> result;
				result.reserve(_storage.size());
				
				_lock.Lock();
				
				for(auto &pair : _storage)
					result.push_back(pair.second);
				
				_lock.Unlock();
				
				return result;
			}
			
			std::vector<T> move_values()
			{
				std::vector<T> result;
				result.reserve(_storage.size());
				
				_lock.Lock();
				
				for(auto &pair : _storage)
					result.push_back(std::move(pair.second));
				
				_storage.clear();
				_lock.Unlock();
				
				return result;
			}
			
			
			T &operator *()
			{
				return get();
			}
			const T &operator *() const
			{
				return get();
			}
			T *operator ->()
			{
				return &get();
			}
			const T *operator ->() const
			{
				return &get();
			}
			
		private:
			mutable SpinLock _lock;
			std::unordered_map<std::thread::id, T> _storage;
		};
	}
}

#endif /* __RAYNE_THREADLOCALSTORAGE_H__ */
