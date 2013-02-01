//
//  RNAutoreleasePool.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
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
		RN_ASSERT(_owner->_pool == this, "Popping pool other than the topmost pool is forbidden!");
		
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
