//
//  RNAsyncCacheable.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCache.h"
#include "RNThreadPool.h"

namespace RN
{
	OpenGLThreadPool<std::function<void ()>> *AsyncCacheableThreadPool()
	{
		static OpenGLThreadPool<std::function<void ()>> *pool;
		static std::once_flag once;
		
		std::call_once(once, [&]() {
			pool = new OpenGLThreadPool<std::function<void ()>>();
		});
		
		return pool;
	}
	
	
	AsyncCacheable::AsyncCacheable(bool hasContent)
	{
		_hasContent = hasContent;
		_isCreatingContent = false;
	}
	
	bool AsyncCacheable::BeginContentAccess()
	{
		_contentLock.Lock();
		if(!_hasContent && !_isCreatingContent)
		{
			_isCreatingContent = true;
			
			AsyncCacheableThreadPool()->AddTask([this]() {
				RecreateContent();
				
				_contentLock.Lock();
				_isCreatingContent = false;
				_hasContent = true;
				_contentLock.Unlock();
			});
		}
		_contentLock.Unlock();
		
		return true;
	}
	
	void AsyncCacheable::EndContentAccess()
	{}
	
	
	bool AsyncCacheable::DiscardContent()
	{
		_contentLock.Lock();
		
		bool canDiscard = (_isCreatingContent == false);
		if(canDiscard)
		{
			DisposeContent();
			_hasContent = false;
		}
		
		_contentLock.Unlock();
		
		return canDiscard;
	}
	
	bool AsyncCacheable::IsDiscarded()
	{
		_contentLock.Lock();
		bool discarded = (!_hasContent && !_isCreatingContent);
		_contentLock.Unlock();
		
		return discarded;
	}
}
