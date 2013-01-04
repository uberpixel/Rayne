//
//  RNThread.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNThread.h"
#include "RNMutex.h"
#include "RNArray.h"
#include "RNContext.h"

#include "RNTexture.h"
#include "RNCamera.h"
#include "RNMesh.h"
#include "RNShader.h"

namespace RN
{
	static Mutex       *__ThreadMutex = 0;
	static ObjectArray *__ThreadArray = 0;
	
#if RN_PLATFORM_POSIX
	pthread_t __ThreadMainThread;
#endif
#if RN_PLATFORM_WINDOWS
	DWORD __ThreadMainThread;
#endif
	
	Thread::Thread(ThreadEntry entry)
	{
		RN_ASSERT0(entry != 0);
		Initialize();
		
		_detached = false;
		_entry = entry;
	}
	
	Thread::Thread()
	{
		Initialize();
		
		_detached = true;
		_entry = 0;
		
#if RN_PLATFORM_POSIX
		_thread = pthread_self();
#endif
#if RN_PLATFORM_WINDOWS
		_thread = GetCurrentThread();
		_id     = GetCurrentThreadId();
#endif
		
		__ThreadMutex->Lock();
		__ThreadArray->AddObject(this);
		__ThreadMutex->Unlock();
	}
	
	Thread::~Thread()
	{
		_mutex->Release();
		
		_textures->Release();
		_cameras->Release();
		_meshes->Release();
		
		if(_context)
			_context->DeactivateContext();
	}
	
	void Thread::Initialize()
	{
		_textures = new ObjectArray();
		_cameras  = new ObjectArray();
		_meshes   = new ObjectArray();
		
		_mutex = new Mutex();
		_context = 0;
		
		RN_ASSERT0(_textures && _cameras && _meshes);
		RN_ASSERT0(_mutex != 0);
	}
	
	Thread *Thread::CurrentThread()
	{
		__ThreadMutex->Lock();
		
		for(machine_uint i=0; i<__ThreadArray->Count(); i++)
		{
			Thread *thread = (Thread *)__ThreadArray->ObjectAtIndex(i);
			if(thread->OnThread())
			{
				__ThreadMutex->Unlock();
				return thread;
			}
		}
		
		__ThreadMutex->Unlock();
		
		Thread *thread = new Thread();
		return thread;
	}
	
	
	void Thread::Exit()
	{
		__ThreadMutex->Lock();
		__ThreadArray->RemoveObject(this);
		__ThreadMutex->Unlock();
	}
	
	void *Thread::Entry(void *object)
	{
		Thread *thread = (Thread *)object;
		
		__ThreadMutex->Lock();
		__ThreadArray->AddObject(thread);
		__ThreadMutex->Unlock();
		
		thread->_entry(thread);
		thread->Exit();
		thread->Release();
		
		return NULL;
	}
	
	bool Thread::OnThread() const
	{
		if(!_detached)
			return false;
		
#if RN_PLATFORM_POSIX
		return pthread_equal(_thread, pthread_self());
#endif
#if RN_PLATFORM_WINDOWS
		return (_id == GetCurrentThreadId());
#endif
	}
	
	void Thread::Detach()
	{
		_mutex->Lock();
		
		if(!_detached)
		{
			Retain();
			_detached = true;
		
#if RN_PLATFORM_POSIX
			int result = pthread_create(&_thread, NULL, &Thread::Entry, this);
			if(result)
			{
				Release();
				_detached = false;
			}
#endif
#if RN_PLATFORM_WINDOWS
			_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&Thread::Entry, this, 0, &_id);
			if(_thread == NULL)
			{
				Release();
				_detached = false;
			}
#endif
		}
		
		_mutex->Unlock();
	}
	
	void Thread::Join(Thread *other)
	{
		other->_mutex->Lock();
		RN_ASSERT0(other->_detached);
		other->_mutex->Unlock();
		
#if RN_PLATFORM_POSIX
		pthread_join(other->_thread, NULL);
#endif
#if RN_PLATFORM_WINDOWS
		WaitForSingleObject(other->_thread, INFINITE);
#endif
	}
	
	void Thread::PushTexture(Texture *texture)
	{
		_textures->AddObject(texture);
	}
	
	void Thread::PushCamera(Camera *camera)
	{
		_cameras->AddObject(camera);
	}
	
	void Thread::PushMesh(Mesh *mesh)
	{
		_meshes->AddObject(mesh);
	}
	
	
	RN_INITIALIZER(__ThreadInitializer)
	{
		__ThreadMutex = new Mutex();
		__ThreadArray = new ObjectArray();
		
#if RN_PLATFORM_POSIX
		__ThreadMainThread = pthread_self();
#endif
#if RN_PLATFORM_WINDOWS
		__ThreadMainThread = GetCurrentThreadId();
#endif
		
		RN_ASSERT0(__ThreadMutex != 0 && __ThreadArray != 0);
	}
}
