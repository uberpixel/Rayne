//
//  RNProxy.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PROXY_H__
#define __RAYNE_PROXY_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNSpinLock.h"

namespace RN
{
	template <typename T>
	class Proxy;
	
	template <typename T>
	class ProxyListener
	{
	public:
		virtual T CopyData(Proxy<T> *proxy) = 0;
		virtual T SimpleAccessData(Proxy<T> *proxy) = 0;
		virtual void FreeData(Proxy<T> *proxy, T data) = 0;
	};
	
	template <typename T>
	class Proxy
	{
	public:
		Proxy(ProxyListener<T> *source)
		{
			_source = source;
			_listener = 0;
			_hasCopy = false;
		}
		
		~Proxy()
		{}
		
		// Change
		void WillChangeData()
		{
			_lock.Lock();
			
			if(!_hasCopy)
				_cache = _source->CopyData(this);
		}
		
		void DidChangeData()
		{
			_lock.Unlock();
		}
		
		// Access
		void WillAccessData()
		{
			_lock.Lock();
		}
		
		void DidAccessData()
		{
			_lock.Unlock();
		}
		
		T AccessData()
		{
			if(_hasCopy)
				return _cache;
			
			return _source->SimpleAccessData(this);
		}
		
		// Listener
		void AddListener()
		{
			_lock.Lock();
			
			_listener ++;
			
			_lock.Unlock();
		}
		
		void RemoveListener()
		{
			_lock.Lock();
			
			if((-- _listener) == 0 && _hasCopy)
			{
				_source->FreeData(this, _cache);
				_hasCopy = false;
			}
		
			_lock.Unlock();
		}
		
	private:
		SpinLock _lock;
		ProxyListener<T> *_source;
		
		uint32 _listener;
		bool _hasCopy;
		T _cache;
	};
}

#endif /* __RAYNE_PROXY_H__ */
