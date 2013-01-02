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
	class Proxy
	{
	public:
		Proxy()
		{
			_listener = 0;
			_hasCopy = false;
		}
		
		~Proxy()
		{}
	
		// Change
		virtual void WillChangeData()
		{
			_lock.Lock();
			
			if(!_hasCopy && _listener > 0)
			{
				CreateCopy();
				_hasCopy = true;
			}
		}
		
		virtual void DidChangeData()
		{
			_lock.Unlock();
		}
		
		// Access
		virtual void WillAccessData()
		{
			_lock.Lock();
		}
		
		virtual void DidAccessData()
		{
			_lock.Unlock();
		}
		
		// Listener
		virtual void AddListener()
		{
			_lock.Lock();
			
			_listener ++;
			
			_lock.Unlock();
		}
		
		virtual void RemoveListener()
		{
			_lock.Lock();
			
			if((-- _listener) == 0 && _hasCopy)
			{
				DisposeCopy();
				_hasCopy = false;
			}
		
			_lock.Unlock();
		}
		
	protected:
		SpinLock _lock;
		uint32 _listener;
		
		virtual void CreateCopy()
		{
		}
		
		virtual void DisposeCopy()
		{
		}
		
		bool ShouldServeCopy()
		{
			return (_hasCopy == true);
		}
		
	private:
		bool _hasCopy;
	};
	
	class BlockingProxy : public Proxy
	{
	public:
		virtual void AddListener()
		{
			if((++ _listener) == 1)
				_lock.Lock();
		}
		
		virtual void RemoveListener()
		{
			if((-- _listener) == 0)
				_lock.Unlock();
		}
	};
}

#endif /* __RAYNE_PROXY_H__ */
