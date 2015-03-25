//
//  RNThread.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNThread.h"
#include "RNThreadLocalStorage.h"

#if RN_PLATFORM_WINDOWS
const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void RNSetThreadName(char *threadName)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = -1;
	info.dwFlags = 0;
	
	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR *)&info);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{}
}
#endif

namespace RN
{
	__RNDefineMetaAndGFYMSVC(Thread, Object)
	
	static Thread *__MainThread;
	static ThreadLocalStorage<Thread *> __LocalThread;
	static std::atomic<uint32> __ThreadAtomicIDs;
	
	Thread::Thread() :
		_id(std::this_thread::get_id()),
		_name(nullptr)
	{
		Initialize();

		_name = new String("Main Thread", true);
		__LocalThread.SetValue(this);
		__MainThread = this;
	}
	
	Thread::~Thread()
	{
#if RN_PLATFORM_MAC_OS
		// Works around a bug in OS X, which sometimes crashes mutexes and condition variables
		// https://devforums.apple.com/thread/220316?tstart=0
		
		struct timespec time { .tv_sec = 0, .tv_nsec = 1 };
		pthread_cond_timedwait_relative_np(_exitSignal.native_handle(), _exitMutex.native_handle(), &time);
#endif

		delete _runLoop;

		_dictionary->Release();
		_name->Release();
	}
	
	void Thread::Initialize()
	{
		//_context = nullptr;
		
		_isRunning   = false;
		_isCancelled = false;
		_isDetached  = false;

		_runLoop = new RunLoop();
		_dictionary = new Dictionary();
		
		Retain();
		//ThreadCoordinator::GetSharedInstance()->ConsumeConcurrency();
	}
	
	Thread *Thread::GetCurrentThread()
	{
		return __LocalThread.GetValue();
	}
	Thread *Thread::GetMainThread()
	{
		return __MainThread;
	}
	
	
	void Thread::WaitForExit()
	{
		if(OnThread())
			return;
		
		Retain();
		
		std::unique_lock<std::mutex> lock(_exitMutex);
		
		if(!IsRunning())
		{
			Release();
			return;
		}
		
		_exitSignal.wait(lock, [&]() { return !IsRunning(); });
		
		Release();
	}

	void Thread::ExecuteOnExit(std::function<void (void *)> &&function, void *context)
	{
		std::lock_guard<std::mutex> lock(_exitMutex);
		__UnscheduleExecuteOnExit(context);
		_exitFunctions.push_back(std::make_pair(std::move(function), context));
	}

	void Thread::UnscheduleExecuteOnExit(void *context)
	{
		std::lock_guard<std::mutex> lock(_exitMutex);
		__UnscheduleExecuteOnExit(context);
	}

	void Thread::__UnscheduleExecuteOnExit(void *context)
	{
		for(auto i = _exitFunctions.begin(); i != _exitFunctions.end(); i ++)
		{
			auto &pair = *i;
			if(pair.second == context)
			{
				_exitFunctions.erase(i);
				return;
			}
		}
	}
	
	void Thread::Entry()
	{
		_id = std::this_thread::get_id();
		__LocalThread.SetValue(this);
		
		_isRunning.store(true);
	}
	
	void Thread::Exit()
	{
		/*if(_context)
		{
			_context->ForceDeactivate();
			_context = nullptr;
		}*/

		__LocalThread.SetValue(nullptr);
		
		{
			std::lock_guard<std::mutex> lock(_exitMutex);
			_isRunning.store(false);
			_exitSignal.notify_all();

			for(auto &pair : _exitFunctions)
				pair.first(pair.second);
		}
		
		//ThreadCoordinator::GetSharedInstance()->RestoreConcurrency();
		Release();
	}
	
	
	void Thread::AutoAssignName()
	{
		std::stringstream stream;
		stream << "RN::Thread " << __ThreadAtomicIDs.fetch_add(1);
		
		_name = new String(stream.str().c_str());
;	}
	
	void Thread::Start()
	{
		if(_isDetached.exchange(true))
			throw Exception(Exception::Type::InconsistencyException, "Can't start already detached thread!");
		
		std::thread([&]() {
			Entry();
			
			try
			{
				{
					std::lock_guard<std::mutex> lock(_generalMutex);
					
#if RN_PLATFORM_MAC_OS
					pthread_setname_np(_name->GetUTF8String());
#endif
#if RN_PLATFORM_LINUX
					pthread_setname_np(pthread_self(), _name->GetUTF8String());
#endif
#if RN_PLATFORM_WINDOWS
					RNSetThreadName(const_cast<char *>(_name->GetUTF8String()));
#endif
				}
				
				_function();
			}
			catch(Exception e)
			{
				//HandleException(e);
			}
			
			Exit();
		}).detach();
	}
	
	
	
	void Thread::Cancel()
	{
		_isCancelled.store(true);
	}
	
	bool Thread::OnThread() const
	{
		return (_id == std::this_thread::get_id());
	}
	
	void Thread::SetName(const String *name)
	{
		std::lock_guard<std::mutex> lock(_generalMutex);

		_name->Release();
		_name = name ? name->Copy() : nullptr;

		if(!_name)
			_name = RNCSTR("")->Retain();
		
		if(IsRunning() && OnThread())
		{
#if RN_PLATFORM_MAC_OS
			pthread_setname_np(_name->GetUTF8String());
#endif
#if RN_PLATFORM_LINUX
			pthread_setname_np(pthread_self(), _name->GetUTF8String());
#endif
#if RN_PLATFORM_WINDOWS
			RNSetThreadName(const_cast<char *>(_name->GetUTF8String()));
#endif
		}
	}
	
	String *Thread::GetName()
	{
		std::lock_guard<std::mutex> lock(_generalMutex);
		String *name = _name->Copy();
		
		return name->Autorelease();
	}
}
