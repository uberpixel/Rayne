//
//  RNOpenGLQueue.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenGLQueue.h"
#include "RNContextInternal.h"
#include "RNThread.h"

namespace RN
{
	RNDeclareSingleton(OpenGLQueue)
	
	OpenGLQueue::OpenGLQueue() :
		_running(false),
		_context(nullptr)
	{
		_thread = new Thread(std::bind(&OpenGLQueue::ProcessCommands, this), false);
		_thread->SetName("OpenGL Queue");
		_thread->Detach();
		
		// Wait for the thread to arrive
		std::unique_lock<std::mutex> lock(_signalLock);
		_signal.wait(lock, [&] { return _thread->IsRunning(); });
	}
	
	OpenGLQueue::~OpenGLQueue()
	{
		_thread->Cancel();
		_signal.notify_one();
		
		_thread->WaitForExit();
		_thread->Release();
		
		SafeRelease(_context);
	}
	
	void OpenGLQueue::Wait(size_t command)
	{
		std::unique_lock<std::mutex> lock(_signalLock);
		
		
		while(_processed < command)
			_waitSignal.wait_for(lock, std::chrono::microseconds(100), [&]{ return (_processed >= command); });
	}
	
	void OpenGLQueue::Wait()
	{
		size_t command = _id.load(std::memory_order_acquire);
		Wait(command);
	}
	
	
	
	std::future<void> OpenGLQueue::SubmitCommand(std::packaged_task<void ()> &&command)
	{
		std::future<void> future = command.get_future();
		
		if(_thread->OnThread())
		{
			command();
			return future;
		}
		
		LockGuard<decltype(_commandLock)> lock(_commandLock);
		
		// Push the command into the command queue
		_id ++;
		while(!_commands.push(std::move(command)))
		{}
		
		// Notify the command gate if needed
		if(!_running)
		{
			std::lock_guard<std::mutex> lock(_signalLock);
			_signal.notify_one();
		}
		
		return future;
	}
	
	void OpenGLQueue::SwitchContext(Context *context)
	{
		bool hadContext = (_context != nullptr);
		
		SubmitCommand([&]{
			
			if(_context)
			{
				_context->DeactivateContext();
				_context->Release();
			}
			
			_context = SafeRetain(context);
			_context->MakeActiveContext();
			
		});
		
		if(!hadContext)
			_signal.notify_one();
		
		Wait();
	}
	
	
	void OpenGLQueue::ProcessCommands()
	{
		_signal.notify_one();
		
		std::unique_lock<std::mutex> lock(_signalLock);
		_signal.wait(lock, [&] { return (_context != nullptr || _thread->IsCancelled()); });
		lock.unlock();
		
		std::packaged_task<void ()> task;
		
		while(!_thread->IsCancelled())
		{
			_running.store(true);
			
			while(_commands.pop(task))
			{
				task();
				_processed.fetch_add(1, std::memory_order_release);
			}
			
			
			lock.lock();
			_running.store(false);
			_waitSignal.notify_all();
			
			if(!_commands.was_empty())
				continue;
			
			_signal.wait(lock, [&] { return (!_commands.was_empty() || _thread->IsCancelled()); });
		}
	}
}


