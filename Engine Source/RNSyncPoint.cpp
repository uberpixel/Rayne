//
//  RNSyncPoint.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <exception>
#include "RNSyncPoint.h"

namespace RN
{
	// ---------------------
	// MARK: -
	// MARK: __sync_state
	// ---------------------
	
	class __sync_state
	{
	public:
		__sync_state() :
			_signaled(false),
			_hasException(false),
			_references(1)
		{}
		
		void retain()
		{
			++ _references;
		}
		
		void release()
		{
			if((-- _references))
				delete this;
		}
		
		void signal_exception(std::exception_ptr e)
		{
			std::lock_guard<std::mutex> lock(_lock);
			RN_ASSERT(_signaled == false, "A __sync_state can only be signaled once!");
			
			if(_references.load() == 1)
			{
				std::rethrow_exception(e);
				_signaled = true;
			}
			else
			{
				_signaled = true;
				_hasException = true;
				_exception = e;
				_condition.notify_all();
			}
		}
		
		void signal()
		{
			std::lock_guard<std::mutex> lock(_lock);
			RN_ASSERT(_signaled == false, "A __sync_state can only be signaled once!");
			
			_signaled = true;
			_condition.notify_all();
		}
		
		void wait()
		{
			std::unique_lock<std::mutex> lock(_lock);
			if(_signaled)
				return;
			
			_condition.wait(lock, [&]{ return (_signaled == true); });
			
			if(_hasException)
				std::rethrow_exception(_exception);
		}
		
		bool signaled()
		{
			std::unique_lock<std::mutex> lock(_lock);
			return _signaled;
		}
		
	private:
		std::atomic<size_t> _references;
		
		bool _hasException;
		bool _signaled;
		
		std::mutex _lock;
		std::condition_variable _condition;
		std::exception_ptr _exception;
	};
	
	// ---------------------
	// MARK: -
	// MARK: sync_point
	// ---------------------
	
	sync_point::sync_point() :
		_shared(nullptr)
	{}
	
	sync_point::sync_point(__sync_state *state) :
		_shared(state)
	{
		state->retain();
	}
	
	sync_point::sync_point(sync_point &&other)
	{
		_shared = other._shared;
		other._shared = nullptr;
	}
	
	sync_point::~sync_point()
	{
		if(_shared)
			_shared->release();
	}
	
	void sync_point::wait()
	{
		_shared->wait();
	}
	
	bool sync_point::signaled()
	{
		return _shared->signaled();
	}
	
	sync_point &sync_point::operator= (sync_point &&other)
	{
		_shared->release();
		_shared = other._shared;
		
		other._shared = nullptr;
		return *this;
	}
	
	// ---------------------
	// MARK: -
	// MARK: syncable
	// ---------------------
	
	syncable::syncable() :
		_state(new __sync_state())
	{}
	
	syncable::~syncable()
	{
		_state->release();
	}
	
	void syncable::signal_exception(std::exception_ptr e)
	{
		_state->signal_exception(e);
	}
	
	void syncable::signal()
	{
		_state->signal();
	}
	
	sync_point syncable::get_sync_point()
	{
		return sync_point(_state);
	}
}
