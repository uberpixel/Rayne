//
//  RNThread.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNThread.h"
#include "../Base/RNBaseInternal.h"
#include "../Objects/RNAutoreleasePool.h"
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
#endif

#if RN_PLATFORM_LINUX || RN_PLATFORM_ANDROID
	#include <sys/prctl.h>
	#include <sys/resource.h>
	#include <sys/syscall.h>
	#include <unistd.h>
#endif

void RN::Thread::SetCurrentThreadName(const char *threadName)
{
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS || RN_PLATFORM_VISIONOS
	pthread_setname_np(threadName);
#endif
#if RN_PLATFORM_LINUX || RN_PLATFORM_ANDROID
	prctl(PR_SET_NAME, threadName); //pthread_setname_np is supposed to work too, but fails with too long names, while prctl just truncates
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
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
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
#if RN_PLATFORM_LINUX || RN_PLATFORM_ANDROID
	static pid_t __MainThreadID = 0;
#endif
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
#if RN_PLATFORM_LINUX || RN_PLATFORM_ANDROID
		__MainThreadID = static_cast<pid_t>(syscall(SYS_gettid));
#endif
	}

	Thread::~Thread()
	{
		delete _runLoop;

		_dictionary->Release();
		_name->Release();
	}

	void Thread::Initialize()
	{
		_isRunning = false;
		_isCancelled = false;
		_isDetached = false;

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

	void Thread::SetCurrentThreadPriority(Priority priority)
	{
		int error = 0;
#if RN_PLATFORM_LINUX || RN_PLATFORM_ANDROID
		// Rayne's main thread can differ from the process leader on Android.
		errno = 0;
		const int niceValue = priority == Priority::High ? getpriority(PRIO_PROCESS, __MainThreadID) : (priority == Priority::Background ? 10 : 0);
		error = errno;
		if(!error && setpriority(PRIO_PROCESS, 0, niceValue) != 0) error = errno;
#elif RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS || RN_PLATFORM_VISIONOS
		const qos_class_t qos = priority == Priority::High ? QOS_CLASS_USER_INITIATED : (priority == Priority::Background ? QOS_CLASS_BACKGROUND : QOS_CLASS_DEFAULT);
		error = pthread_set_qos_class_self_np(qos, 0);
#elif RN_PLATFORM_WINDOWS
		const int threadPriority = priority == Priority::High ? THREAD_PRIORITY_ABOVE_NORMAL : (priority == Priority::Background ? THREAD_PRIORITY_BELOW_NORMAL : THREAD_PRIORITY_NORMAL);
		if(!::SetThreadPriority(::GetCurrentThread(), threadPriority)) error = static_cast<int>(::GetLastError());
#endif
		if(!error) return;

		AutoreleasePool pool;
		Thread *thread = GetCurrentThread();
		const char *threadName = thread ? thread->GetName()->GetUTF8String() : "external thread";
		const char *priorityName = priority == Priority::High ? "High" : (priority == Priority::Background ? "Background" : "Default");
		if(Logger::GetSharedInstance())
			RNWarningf("Failed setting %s priority for %s: OS error %d", priorityName, threadName, error);
		else
			fprintf(stderr, "Failed setting %s priority for %s: OS error %d\n", priorityName, threadName, error);
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

	void Thread::ExecuteOnExit(std::function<void(void *)> &&function, void *context)
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
		for(auto i = _exitFunctions.begin(); i != _exitFunctions.end(); i++)
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
		;
	}

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

					SetCurrentThreadName(_name->GetUTF8String());
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
			SetCurrentThreadName(_name->GetUTF8String());
		}
	}

	String *Thread::GetName()
	{
		LockGuard<Lockable> lock(_generalMutex);
		String *name = _name->Copy();

		return name->Autorelease();
	}
} // namespace RN
