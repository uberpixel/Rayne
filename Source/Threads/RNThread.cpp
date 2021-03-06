//
//  RNThread.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNThread.h"
#include "RNThreadLocalStorage.h"
#include "../Base/RNBaseInternal.h"
#include "../Objects/RNAutoreleasePool.h"

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
#endif

void RNSetThreadName(char *threadName)
{
#if RN_PLATFORM_MAC_OS
		pthread_setname_np(threadName);
#endif
#if RN_PLATFORM_LINUX || RN_PLATFORM_ANDROID
		pthread_setname_np(pthread_self(), threadName);
#endif
#if RN_PLATFORM_WINDOWS
	#if RN_COMPILER_MSVC
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
	#endif
#endif

#if RN_ENABLE_VTUNE
		__itt_thread_set_nameA(threadName);
#endif
}

namespace RN
{
	namespace __Private
	{
		extern void CleanThreadData();
	}

	__RNDefineMetaAndGFYMSVC(Thread, Object)
	
	static Thread *__MainThread;
	static ThreadLocalStorage<Thread *> __LocalThread;
	static std::atomic<uint32> __ThreadAtomicIDs;
	
	Thread::Thread() :
		_name(nullptr),
		_id(std::this_thread::get_id())
	{
		Initialize();

		_name = new String("RN::Main", true);
		__LocalThread.SetValue(this);
		__MainThread = this;
	}
	
	Thread::~Thread()
	{
		delete _runLoop;

		_dictionary->Release();
		_name->Release();
	}
	
	void Thread::Initialize()
	{
		_isRunning   = false;
		_isCancelled = false;
		_isDetached  = false;

		_runLoop = new RunLoop();
		_dictionary = new Dictionary();
		
		Retain();
	}
	
	Thread *Thread::GetCurrentThread()
	{
		return __LocalThread.GetValue();
	}
	Thread *Thread::GetMainThread()
	{
		return __MainThread;
	}

	void Thread::CleanUp()
	{
		__Private::CleanThreadData();
	}
	
	void Thread::WaitForExit()
	{
		RN_ASSERT(!OnThread(), "Thread::WaitForExit() must not be called from the thread itself");
		
		Retain();
		
		UniqueLock<Lockable> lock(_exitMutex);
		
		if(!IsRunning())
		{
			Release();
			return;
		}
		
		_exitSignal.Wait(lock, [&]() { return !IsRunning(); });
		
		Release();
	}

	void Thread::ExecuteOnExit(std::function<void (void *)> &&function, void *context)
	{
		LockGuard<Lockable> lock(_exitMutex);
		__UnscheduleExecuteOnExit(context);
		_exitFunctions.push_back(std::make_pair(std::move(function), context));
	}

	void Thread::UnscheduleExecuteOnExit(void *context)
	{
		LockGuard<Lockable> lock(_exitMutex);
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
		__LocalThread.SetValue(nullptr);
		
		{
			LockGuard<Lockable> lock(_exitMutex);
			_isRunning.store(false);
			_exitSignal.NotifyAll();

			for(auto &pair : _exitFunctions)
				pair.first(pair.second);
		}

		CleanUp();
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
			throw InconsistencyException("Can't start already detached thread!");
		
		_thread = std::thread([&]() {
			Entry();

			try
			{
				{
					LockGuard<Lockable> lock(_generalMutex);
					AutoreleasePool pool;
					
					RNSetThreadName(const_cast<char *>(_name->GetUTF8String()));
				}
				
				_function();
			}
			catch(Exception e)
			{
				//HandleException(e);
			}
			
			Exit();
		});

		_thread.detach();
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
		LockGuard<Lockable> lock(_generalMutex);

		_name->Release();
		_name = SafeCopy(name);

		if(!_name)
			_name = RNCSTR("")->Retain();
		
		if(IsRunning() && OnThread())
		{
			RNSetThreadName(const_cast<char *>(_name->GetUTF8String()));
		}
	}
	
	String *Thread::GetName()
	{
		LockGuard<Lockable> lock(_generalMutex);
		String *name = _name->Copy();
		
		return name->Autorelease();
	}
}
