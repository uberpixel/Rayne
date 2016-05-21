//
//  RNParkingLot.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNParkingLot.h"
#include "RNThreadLocalStorage.h"
#include "RNSpinLock.h"

namespace RN
{
	struct ThreadData
	{
		ThreadData()  :
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

	static SpinLock _threadLock;
	static std::unordered_map<void *, ThreadData *> _threadQueue;
	static ThreadLocalStorage<ThreadData *> *_threadData;


	ThreadData *GetThreadData()
	{
		static std::once_flag flag;
		std::call_once(flag, []{
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
		_threadLock.Lock();

		auto iterator = _threadQueue.find(const_cast<void *>(address));
		if(iterator == _threadQueue.end())
		{
			_threadQueue.insert({ const_cast<void *>(address), data });
			_threadLock.Unlock();

			return;
		}

		ThreadData *temp = iterator->second;
		data->next = temp->next;
		temp->next = data;

		_threadLock.Unlock();
	}

	void DequeueThreadData(const void *address, ThreadData *data)
	{
		_threadLock.Lock();

		auto iterator = _threadQueue.find(const_cast<void *>(address));
		if(iterator == _threadQueue.end())
		{
			// Umm... ?!

			_threadLock.Unlock();
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
					if(temp->next)
						_threadQueue.insert({ const_cast<void *>(address), temp->next });
					else
						_threadQueue.erase(iterator);

					return;
				}

				prev->next = temp->next;
				return;
			}

			prev = temp;
			temp = temp->next;
		}

		_threadLock.Unlock();
	}

	bool ParkingLot::__ParkConditionally(const void *address, std::function<bool()> validation, std::function<void()> beforeSleep, Clock::time_point timeout)
	{
		if(!validation())
			return false;

		ThreadData *data = GetThreadData();

		RN_ASSERT(data->address == nullptr, "Recursively called ParkingLot::Park()");
		data->address = address;

		QueueThreadData(address, data);

		beforeSleep();

		bool gotQueued;

		{
			std::unique_lock<std::mutex> lock(data->lock);
			while(data->address && Clock::now() < timeout)
			{
				if(timeout == Clock::time_point::max())
					data->condition.wait(lock);
				else
					data->condition.wait_until(lock, timeout);

				lock.lock();
				lock.unlock();
			}

			gotQueued = (data->address == nullptr);
		}

		if(gotQueued)
			return true;

		DequeueThreadData(address, data);

		{
			std::unique_lock<std::mutex> lock(data->lock);
			data->address = nullptr;
		}

		return false;
	}

	void ParkingLot::UnparkThread(const void *address, std::function<void(UnparkResult)> callback)
	{
		_threadLock.Lock();

		auto iterator = _threadQueue.find(const_cast<void *>(address));
		if(iterator == _threadQueue.end())
		{
			callback(0);

			_threadLock.Unlock();
			return;
		}

		ThreadData *prev = nullptr;
		ThreadData *data = iterator->second;
		while(data)
		{
			if(data->address == address)
			{
				UnparkResult result = UnparkResult::UnparkedThread;

				if(data->next)
					result |= UnparkResult::HasMoreThreads;

				callback(result);

				break;
			}

			prev = data;
			data = data->next;
		}

		if(!data)
		{
			callback(0);

			_threadLock.Unlock();
			return;
		}

		// Remove the ThreadData from the queue
		if(prev)
		{
			prev->next = data->next;
		}
		else
		{
			if(data->next)
				_threadQueue.insert({ const_cast<void *>(address), data->next });
			else
				_threadQueue.erase(iterator);
		}

		_threadLock.Unlock();


		// Clear the address
		{
			std::unique_lock<std::mutex> lock(data->lock);
			data->address = nullptr;
		}

		data->condition.notify_one();
	}

	ParkingLot::UnparkResult ParkingLot::UnparkThread(const void *address)
	{
		UnparkResult result;

		UnparkThread(address, [&](UnparkResult res) {
			result = res;
		});

		return result;
	}

	void ParkingLot::UnparkAllThreads(const void *address)
	{
		_threadLock.Lock();

		auto iterator = _threadQueue.find(const_cast<void *>(address));
		if(iterator == _threadQueue.end())
		{
			_threadLock.Unlock();
			return;
		}

		std::vector<ThreadData *> threads;

		{
			ThreadData *data = iterator->second;
			while(data)
			{
				threads.push_back(data);
				data = data->next;
			}
		}

		_threadQueue.erase(iterator);
		_threadLock.Unlock();

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
