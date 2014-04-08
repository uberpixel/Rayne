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
	RNDefineSingleton(OpenGLQueue)
	
	OpenGLQueue::OpenGLQueue() :
		_running(false),
		_context(nullptr),
		_processed(0),
		_id(0)
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
	
	
	
	void OpenGLQueue::RunTask(Task &task)
	{
		try
		{
			task.task();
			task.syncable.signal();
		}
		catch(...)
		{
			task.syncable.signal_exception(std::current_exception());
		}
	}
	
	stl::sync_point OpenGLQueue::SubmitCommand(Task &&command)
	{
		stl::sync_point point = command.syncable.get_sync_point();
		
		if(_thread->OnThread())
		{
			command.task();
			command.syncable.signal();
			
			return point;
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
		
		return point;
	}
	
	void OpenGLQueue::SwitchContext(Context *context)
	{
		bool hadContext = (_context != nullptr);
		if(!hadContext)
		{
			std::unique_lock<std::mutex> lock(_signalLock);
			_context = SafeRetain(context);
			_signal.notify_one();
		}
		else
		{
			SubmitCommand([&]{
				
				if(_context)
				{
					_context->DeactivateContext();
					_context->Release();
				}
				
				_context = SafeRetain(context);
				_context->MakeActiveContext();
				
			});
		}
		
		Wait();
	}
	
	
	void OpenGLQueue::ProcessCommands()
	{
		_signal.notify_one();
		
		std::unique_lock<std::mutex> lock(_signalLock);
		_signal.wait(lock, [&] { return (_context != nullptr || _thread->IsCancelled()); });
		_context->MakeActiveContext();
		
		Task task;
		
		try
		{
			while(!_thread->IsCancelled())
			{
				_running.store(true);
				lock.unlock();
				
				while(_commands.pop(task))
				{
					RunTask(task);
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
		catch(Exception e)
		{
			HandleException(e);
		}
	}
}


