//
//  RNAutoreleasePool.cpp
//  Rayne
//
//  Created by Sidney Just on 31.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "RNAutoreleasePool.h"

namespace RN
{
	AutoreleasePool::AutoreleasePool()
	{
		_owner  = Thread::CurrentThread();
		_parent = AutoreleasePool::CurrentPool();
		
		_owner->_pool = this;
	}
	
	AutoreleasePool::~AutoreleasePool()
	{
		Drain();
		_owner->_pool = _parent;
	}
	
	void AutoreleasePool::AddObject(Object *object)
	{
		_objects.push_back(object);
	}
	
	void AutoreleasePool::Drain()
	{
		for(auto i=_objects.begin(); i!=_objects.end(); i++)
		{
			Object *object = *i;
			object->Release();
		}
		
		_objects.clear();
	}
	
	AutoreleasePool *AutoreleasePool::CurrentPool()
	{
		Thread *thread = Thread::CurrentThread();
		return thread->_pool;
	}
}
