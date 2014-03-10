//
//  RNMessage.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMessage.h"

namespace RN
{
	RNDefineMeta(Message)
	RNDefineSingleton(MessageCenter)
	
	Message::Message(String *name, Object *object, Dictionary *info) :
		_name(SafeRetain(name)),
		_object(SafeRetain(object)),
		_info(SafeRetain(info))
	{}
	
	Message::~Message()
	{
		SafeRelease(_name);
		SafeRelease(_object);
		SafeRelease(_info);
	}
	
	
	
	void MessageCenter::PostMessage(Message *message)
	{
		LockGuard<SpinLock> lock(_lock);
		
		auto iterator = _observer.find(message->GetName());
		if(iterator != _observer.end())
		{
			std::vector<MessageObserverProxy> observer = iterator->second;
			lock.Unlock();
			
			for(MessageObserverProxy &proxy : observer)
			{
				proxy.callback(message);
			}
		}
	}
	
	void MessageCenter::PostMessage(String *name, Object *object, Dictionary *info)
	{
		Message *message = new Message(name, object, info);
		PostMessage(message);
		message->Release();
	}
	
	
	
	
	void MessageCenter::AddObserver(String *name, CallbackType callback, void *cookie)
	{
		MessageObserverProxy proxy;
		proxy.cookie = cookie;
		proxy.callback = std::move(callback);
		
		LockGuard<SpinLock> lock(_lock);
		
		auto iterator = _observer.find(name);
		if(iterator != _observer.end())
		{
			std::vector<MessageObserverProxy> &observer = iterator->second;
			observer.push_back(std::move(proxy));
		}
		else
		{
			std::vector<MessageObserverProxy> observer;
			observer.push_back(std::move(proxy));
			
			_observer.emplace(std::make_pair(name->Copy(), std::move(observer)));
		}
	}
	
	void MessageCenter::RemoveObserver(void *cookie)
	{
		LockGuard<SpinLock> lock(_lock);
		
		std::vector<String *> emptyPaths;
		
		for(auto i = _observer.begin(); i != _observer.end(); i ++)
		{
			std::vector<MessageObserverProxy> &observer = i->second;
			
			for(auto j = observer.begin(); j != observer.end();)
			{
				if(j->cookie == cookie)
				{
					j = observer.erase(j);
					continue;
				}
				
				j ++;
			}
			
			if(observer.empty())
				emptyPaths.push_back(i->first);
		}
		
		if(!emptyPaths.empty())
		{
			for(String *name : emptyPaths)
			{
				_observer.erase(name);
				name->Release();
			}
		}
	}
	
	void MessageCenter::RemoveObserver(void *cookie, String *name)
	{
		LockGuard<SpinLock> lock(_lock);
		
		auto iterator = _observer.find(name);
		if(iterator != _observer.end())
		{
			std::vector<MessageObserverProxy> &observer = iterator->second;
			
			for(auto i = observer.begin(); i != observer.end();)
			{
				if(i->cookie == cookie)
				{
					i = observer.erase(i);
					continue;
				}
				
				i ++;
			}
			
			if(observer.empty())
			{
				iterator->first->Release();
				_observer.erase(iterator);
			}
		}
	}
}
