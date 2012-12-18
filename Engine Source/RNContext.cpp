//
//  RNContext.cpp
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNContext.h"
#include "RNMutex.h"

namespace RN
{
	void Context::MakeActiveContext()
	{
		Thread *thread = Thread::CurrentThread();
		RN::Assert(thread);
		
		thread->_mutex->Lock();
		
		if(thread->_context)
		{
			Context *other = thread->_context;
			other->_active = false;
			other->_thread = 0;
			
			other->Flush();
			other->Deactivate();
		}
		
		this->Activate();
		
		this->_active = true;
		this->_thread = thread;
		
		thread->_context = this;
		thread->_mutex->Unlock();
	}
	
	void Context::DeactiveContext()
	{
		Thread *thread = Thread::CurrentThread();
		RN::Assert(thread);
		
		thread->_mutex->Lock();
		
		this->_active = false;
		this->_thread = 0;
		
		this->Flush();
		this->Deactivate();
		
		thread->_context = 0;
		thread->_mutex->Unlock();
	}
	
	Context *Context::ActiveContext()
	{
		Thread *thread = Thread::CurrentThread();
		RN::Assert(thread);
		
		return thread->_context;
	}
}
