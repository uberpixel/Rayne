//
//  RNThread.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNThread.h"
#include "RNSpinLock.h"
#include "RNArray.h"
#include "RNContext.h"

#include "RNTexture.h"
#include "RNCamera.h"
#include "RNMesh.h"
#include "RNShader.h"
#include "RNThreadPool.h"

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
		
		std::lock_guard<std::mutex> lock(_exitMutex);
		_exitSignal.notify_all();
		
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
		if(_isRunning.exchange(true) == true)
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
				
				_mutex.Unlock();
				
				_function();
			}
			catch(Exception e)
			{
				__HandleException(e);
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
	
	uint32 Thread::SetOpenGLBinding(GLenum target, GLuint object)
	{
		auto iterator = _glBindings.find(target);
		
		if(iterator != _glBindings.end())
		{
			std::tuple<GLuint, uint32>& tuple = iterator->second;
			
			if(object != 0)
			{
				if(std::get<0>(tuple) != object)
					throw Exception(Exception::Type::InconsistencyException, "Tried to retain a binding that doesn't exist!");
				
				uint32& references = std::get<1>(tuple);
				return references;
			}
			else
			{
				uint32& references = std::get<1>(tuple);
				
				if((-- references) == 0)
				{
					_glBindings.erase(iterator);
					return 0;
				}
				
				return references;
			}
		}
		
		if(object == 0)
			return 0;
		
		_glBindings.insert(std::unordered_map<GLenum, std::tuple<GLuint, uint32>>::value_type(target, std::make_tuple(object, 1)));
		return 1;
	}
	
	GLuint Thread::GetOpenGLBinding(GLenum target)
	{
		auto iterator = _glBindings.find(target);
		return (iterator != _glBindings.end()) ? std::get<0>(iterator->second) : 0;
	}
}
