//
//  RNPThreadPark.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNFutex.h"
#include "RNThreadLocalStorage.h"
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
		static ThreadLocalStorage<ThreadData *> *_threadData;


		ThreadData *GetThreadData()
		{
			static std::once_flag flag;
			std::call_once(flag, [] {
				_threadData = new ThreadLocalStorage<ThreadData *>();
			});

			ThreadData *data = _threadData->GetValue();

			if(RN_EXPECT_FALSE(!data))
			{
				data = new ThreadData();
				_threadData->SetValue(data);
			}

			return data;
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

			auto iterator = _threadQueue.find(const_cast<void *>(address));
			if(iterator == _threadQueue.end())
			{
				// Umm... ?!
				return;
			}


			ThreadData *prev = nullptr;
			ThreadData *temp = iterator->second;
			while(temp)
			{
				if(temp == data)
				{
					if(!prev)
					{
						_threadQueue.erase(iterator);

						if(temp->next)
							_threadQueue.insert({const_cast<void *>(address), temp->next});

						temp->next = nullptr;
						return;
					}

					prev->next = temp->next;
					temp->next = nullptr;
					return;
				}

				prev = temp;
				temp = temp->next;
			}

			data->next = nullptr;
		}

		bool Futex::__WaitConditionally(const void *address, std::function<bool()> validation, std::function<void()> beforeSleep, Clock::time_point timeout)
		{
			ThreadData *data = GetThreadData();

			{
				std::unique_lock<std::mutex> lock(data->lock);
				RN_ASSERT(data->address == nullptr, "Recursively called Futex::Wait()");

				data->address = address;
			}

			if(!validation())
			{
				std::unique_lock<std::mutex> lock(data->lock);
				data->address = nullptr;

				beforeSleep();
				return false;
			}

			QueueThreadData(address, data);

			beforeSleep();

			{
				std::unique_lock<std::mutex> lock(data->lock);
				while(data->address && Clock::now() < timeout)
				{
					if(timeout == Clock::time_point::max())
						data->condition.wait(lock);
					else
						data->condition.wait_until(lock, timeout);

					lock.unlock();
					lock.lock();
				}

				RN_DEBUG_ASSERT(!data->address || data->address == address, "Invalid data address");
				if(!data->address)
					return true;
			}

			DequeueThreadData(address, data);

			{
				std::unique_lock<std::mutex> lock(data->lock);
				data->address = nullptr;
			}

			return false;
		}

		void Futex::WakeOne(const void *address, std::function<void(WakeResult)> callback)
		{
			ThreadData *data;

			{
				LockGuard<SpinLock> lock(_threadLock);

				auto iterator = _threadQueue.find(const_cast<void *>(address));
				if(iterator == _threadQueue.end())
				{
					callback(0);
					return;
				}

				data = iterator->second;
				while(data)
				{
					if(data->address == nullptr)
					{
						data = data->next;
						continue;
					}

					RN_DEBUG_ASSERT(data->address == address, "Invalid data address");

					WakeResult result = WakeResult::WokeUpThread;

					ThreadData *temp = data->next;
					while(temp)
					{
						if(temp->address == address)
						{
							result |= WakeResult::HasMoreThreads;
							break;
						}

						temp = temp->next;
					}

					callback(result);
					break;
				}

				if(!data)
				{
					callback(0);
					return;
				}

				// Remove the ThreadData from the queue
				_threadQueue.erase(iterator);

				if(data->next)
					_threadQueue.insert({const_cast<void *>(address), data->next});

				data->next = nullptr;
			}

			// Clear the address
			{
				std::unique_lock<std::mutex> lock(data->lock);
				data->address = nullptr;
			}

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
			std::vector<ThreadData *> threads;

			{
				LockGuard<SpinLock> lock(_threadLock);

				auto iterator = _threadQueue.find(const_cast<void *>(address));
				if(iterator == _threadQueue.end())
					return;

				{
					ThreadData *data = iterator->second;
					while(data)
					{
						threads.push_back(data);
						data = data->next;
					}
				}

				_threadQueue.erase(iterator);
			}

			for(ThreadData *data : threads)
			{
				{
					std::unique_lock<std::mutex> lock(data->lock);
					data->address = nullptr;
				}

				data->condition.notify_one();
			}
		}
	}
}
