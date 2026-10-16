//
//  RNPThreadPark.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNFutex.h"
#include "../Base/RNBase.h"
#include "RNLockable.h"

namespace RN
{
	namespace __Private
	{
		struct ThreadData
		{
			ThreadData() :
				address(nullptr),
				threadId(std::this_thread::get_id()),
				next(nullptr)
			{}

			std::mutex lock;
			std::condition_variable condition;

			const void *address;
			std::thread::id threadId;

			ThreadData *next;
		};

		class SpinLock
		{
		public:
			SpinLock()
			{
				_flag.clear();
			}

			void Lock()
			{
				while(_flag.test_and_set(std::memory_order_acquire))
				{}
			}

			void Unlock()
			{
				_flag.clear(std::memory_order_release);
			}

			bool TryLock()
			{
				return (!_flag.test_and_set(std::memory_order_acquire));
			}

		private:
			std::atomic_flag _flag;
		};

		static SpinLock _threadLock;
		static std::unordered_map<void *, ThreadData *> _threadQueue;

		inline std::unique_ptr<ThreadData> &GetThreadDataStorage()
		{
			static thread_local std::unique_ptr<ThreadData> data;
			return data;
		}

		ThreadData *GetThreadData()
		{
			auto &data = GetThreadDataStorage();
			if(RN_EXPECT_FALSE(!data))
				data.reset(new ThreadData());

			return data.get();
		}

		void CleanThreadData()
		{
			GetThreadDataStorage().reset();
		}

		void QueueThreadData(const void *address, ThreadData *data)
		{
			LockGuard<SpinLock> lock(_threadLock);

			auto iterator = _threadQueue.find(const_cast<void *>(address));
			if(iterator == _threadQueue.end())
			{
				_threadQueue.insert({const_cast<void *>(address), data});
				data->next = nullptr;

				return;
			}

			ThreadData *temp = iterator->second;

			while(temp)
			{
				if(!temp->next)
				{
					data->next = nullptr;
					temp->next = data;

					return;
				}

				temp = temp->next;
			}
		}

		void DequeueThreadData(const void *address, ThreadData *data)
		{
			LockGuard<SpinLock> lock(_threadLock);
			std::unique_lock<std::mutex> dataLock(data->lock);

			auto iterator = _threadQueue.find(const_cast<void *>(address));
			if(iterator != _threadQueue.end())
			{
				ThreadData **link = &iterator->second;
				while(*link && *link != data)
					link = &(*link)->next;

				if(*link)
				{
					*link = data->next;
					if(!iterator->second)
						_threadQueue.erase(iterator);
				}
			}

			data->next = nullptr;
			data->address = nullptr;
		}

		bool Futex::__WaitConditionally(const void *address, std::function<bool()> validation, std::function<void()> beforeSleep, Clock::time_point timeout)
		{
			return Wait(address, validation, beforeSleep, timeout);
		}

		bool Futex::__WaitBorrowed(const void *address, void *validationContext, bool (*validation)(void *), void *beforeSleepContext, void (*beforeSleep)(void *), Clock::time_point timeout)
		{
			ThreadData *data = nullptr;
			try
			{
				data = GetThreadData();
			}
			catch(const std::bad_alloc &)
			{}
			catch(const std::system_error &)
			{}

			if(data)
			{
				{
					std::unique_lock<std::mutex> lock(data->lock);
					RN_ASSERT(data->address == nullptr, "Recursively called Futex::Wait()");
					data->address = address;
				}

				try
				{
					QueueThreadData(address, data);
				}
				catch(const std::bad_alloc &)
				{
					std::unique_lock<std::mutex> lock(data->lock);
					data->address = nullptr;
					data = nullptr;
				}
			}

			if(!data)
			{
				// No validation has published a parked state. Retry as a spurious wake.
				beforeSleep(beforeSleepContext);
				std::this_thread::yield();
				return false;
			}

			bool waitFailed = false;
			try
			{
				if(!validation(validationContext))
				{
					DequeueThreadData(address, data);
					beforeSleep(beforeSleepContext);
					return false;
				}

				beforeSleep(beforeSleepContext);

				std::unique_lock<std::mutex> lock(data->lock);
				while(data->address && Clock::now() < timeout)
				{
					try
					{
						if(timeout == Clock::time_point::max())
							data->condition.wait(lock);
						else
							data->condition.wait_until(lock, timeout);
					}
					catch(const std::system_error &)
					{
						waitFailed = true;
						break;
					}
				}

				RN_DEBUG_ASSERT(!data->address || data->address == address, "Invalid data address");
				if(!data->address)
					return true;
			}
			catch(...)
			{
				DequeueThreadData(address, data);
				throw;
			}

			DequeueThreadData(address, data);
			if(waitFailed)
				std::this_thread::yield();
			return false;
		}

		void Futex::WakeOne(const void *address, std::function<void(WakeResult)> callback)
		{
			WakeOne<std::function<void(WakeResult)> &>(address, callback);
		}

		void Futex::__WakeOneBorrowed(const void *address, void *context, void (*callback)(void *, WakeResult))
		{
			LockGuard<SpinLock> lock(_threadLock);
			auto iterator = _threadQueue.find(const_cast<void *>(address));
			if(iterator == _threadQueue.end())
			{
				callback(context, 0);
				return;
			}

			ThreadData *data = iterator->second;
			std::unique_lock<std::mutex> dataLock(data->lock);
			RN_DEBUG_ASSERT(data->address == address, "Invalid data address");

			WakeResult result = WakeResult::WokeUpThread;
			if(data->next)
				result |= WakeResult::HasMoreThreads;
			callback(context, result);

			iterator->second = data->next;
			if(!iterator->second)
				_threadQueue.erase(iterator);

			data->next = nullptr;
			data->address = nullptr;
			data->condition.notify_one();
		}

		Futex::WakeResult Futex::WakeOne(const void *address)
		{
			WakeResult result;

			WakeOne(address, [&](WakeResult res) {
				result = res;
			});

			return result;
		}

		void Futex::WakeAll(const void *address)
		{
			LockGuard<SpinLock> lock(_threadLock);
			auto iterator = _threadQueue.find(const_cast<void *>(address));
			if(iterator == _threadQueue.end())
				return;

			while(iterator->second)
			{
				ThreadData *data = iterator->second;
				std::unique_lock<std::mutex> dataLock(data->lock);
				iterator->second = data->next;
				data->next = nullptr;
				data->address = nullptr;
				data->condition.notify_one();
			}

			_threadQueue.erase(iterator);
		}

	} // namespace __Private
} // namespace RN
