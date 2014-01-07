//
//  RNThread.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNThread.h"
#include "RNSpinLock.h"
#include "RNArray.h"
#include "RNContextInternal.h"

#include "RNTexture.h"
#include "RNCamera.h"
#include "RNMesh.h"
#include "RNShader.h"
#include "RNThreadPool.h"

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
	RNDeclareMeta(Thread)
	
	static SpinLock __ThreadLock;
	static std::unordered_map<std::thread::id, Thread *> __ThreadMap;
	static std::atomic<uint32> __ThreadAtomicIDs;
	
	Thread::Thread()
	{
		Initialize();
		
		_id = std::this_thread::get_id();
		_name = std::string("Main Thread");
		
		__ThreadLock.Lock();
		__ThreadMap[_id] = this;
		__ThreadLock.Unlock();
	}
	
	Thread::~Thread()
	{}
	
	void Thread::Initialize()
	{
		_context = 0;
		_pool    = 0;
		
		_isRunning   = false;
		_isCancelled = false;
		_isDetached  = false;
		
		Retain();
		ThreadCoordinator::GetSharedInstance()->ConsumeConcurrency();
	}
	
	Thread *Thread::GetCurrentThread()
	{
		Thread *thread = 0;
		
		__ThreadLock.Lock();
		
		auto iterator = __ThreadMap.find(std::this_thread::get_id());
		
		if(iterator != __ThreadMap.end())
			thread = iterator->second;
		
		__ThreadLock.Unlock();
		
		return thread;
	}
	
	
	void Thread::WaitForExit()
	{
		if(!IsRunning() || OnThread())
			return;
		
		Retain();
		
		std::unique_lock<std::mutex> lock(_exitMutex);
		_exitSignal.wait(lock, [&]() { return !IsRunning(); });
		
		Release();
	}
	
	void Thread::Entry()
	{
		_id = std::this_thread::get_id();
		
		__ThreadLock.Lock();
		__ThreadMap[_id] = this;
		__ThreadLock.Unlock();
		
		_isRunning.store(true);
	}
	
	void Thread::Exit()
	{
		if(_context)
		{
			_context->ForceDeactivate();
			_context = nullptr;
		}
		
		__ThreadLock.Lock();
		
		auto iterator = __ThreadMap.find(_id);
		__ThreadMap.erase(iterator);
		
		__ThreadLock.Unlock();
		_isRunning.store(false);
		
		{
			std::lock_guard<std::mutex> lock(_exitMutex);
			_exitSignal.notify_all();
		}
		
		ThreadCoordinator::GetSharedInstance()->RestoreConcurrency();
		Release();
	}
	
	
	void Thread::AutoAssignName()
	{
		std::stringstream stream;
		stream << "RN::Thread " << __ThreadAtomicIDs.fetch_add(1);
		
		_name = stream.str();
	}
	
	void Thread::Detach()
	{
		if(_isDetached.exchange(true) == true)
			throw Exception(Exception::Type::InconsistencyException, "Can't detach already detached thread!");
		
		std::thread([&]() {
			Entry();
			
			try
			{
				_mutex.Lock();
				
#if RN_PLATFORM_MAC_OS
				pthread_setname_np(_name.c_str());
#endif
#if RN_PLATFORM_LINUX
				pthread_setname_np(pthread_self(), _name.c_str());
#endif
#if RN_PLATFORM_WINDOWS
				RNSetThreadName(const_cast<char*>(_name.c_str()));
#endif
				
				_mutex.Unlock();
				
				_function();
			}
			catch(Exception e)
			{
				HandleException(e);
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
		__ThreadLock.Lock();
		
		auto iterator = __ThreadMap.find(_id);
		bool onThread = (iterator->second == this);
		
		__ThreadLock.Unlock();
		
		return onThread;
	}
	
	void Thread::SetName(const std::string& name)
	{
		_mutex.Lock();
		_name = std::string(name);
		_mutex.Unlock();
	}
	
	const std::string Thread::GetName()
	{
		_mutex.Lock();
		std::string name = _name;
		_mutex.Unlock();
		
		return name;
	}
}
