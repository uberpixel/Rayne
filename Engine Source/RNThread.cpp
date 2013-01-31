//
//  RNThread.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNThread.h"
#include "RNSpinLock.h"
#include "RNMutex.h"
#include "RNArray.h"
#include "RNContext.h"

#include "RNTexture.h"
#include "RNCamera.h"
#include "RNMesh.h"
#include "RNShader.h"

namespace RN
{
	static SpinLock __ThreadLock;
	static std::unordered_map<std::thread::id, Thread *> __ThreadMap;
	
	Thread::Thread(ThreadEntry entry)
	{
		RN_ASSERT0(entry != 0);
		Initialize();
		
		_detached = false;
		_entry = entry;
		_pool = 0;
	}
	
	Thread::Thread()
	{
		Initialize();
		
		_detached = true;
		_entry = 0;
		_pool = 0;
		
		_id = std::this_thread::get_id();
		
		__ThreadLock.Lock();
		__ThreadMap[_id] = this;
		__ThreadLock.Unlock();
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
		Thread *thread = 0;
		
		__ThreadLock.Lock();
		
		auto iterator = __ThreadMap.find(std::this_thread::get_id());
		if(iterator != __ThreadMap.end())
		{
			thread = iterator->second;
		}
		
		__ThreadLock.Unlock();
		
		if(!thread)
			thread = new Thread();
		
		return thread;
	}
	
	
	void Thread::Exit()
	{
		__ThreadLock.Lock();
		
		auto iterator = __ThreadMap.find(std::this_thread::get_id());
		__ThreadMap.erase(iterator);
		
		__ThreadLock.Unlock();
	}
	
	void Thread::Entry()
	{
		_id = std::this_thread::get_id();
		
		__ThreadLock.Lock();
		__ThreadMap[_id] = this;
		__ThreadLock.Unlock();
		
		_entry(this);
		
		Exit();
		Release();
	}
	
	bool Thread::OnThread() const
	{
		if(!_detached)
			return false;
		
		__ThreadLock.Lock();
		
		auto iterator = __ThreadMap.find(_id);
		bool onThread = (iterator->second == this);
		
		__ThreadLock.Unlock();
		
		return onThread;
	}
	
	void Thread::Detach()
	{
		_mutex->Lock();
		
		if(!_detached)
		{
			Retain();
			_detached = true;
			
			std::thread thread = std::thread(&Thread::Entry, this);
			thread.detach();
		}
		
		_mutex->Unlock();
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
}
