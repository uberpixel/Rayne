//
//  RNMessage.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMessage.h"

namespace RN
{
	Message::Message(MessageGroup group, MessageSubgroup subgroup) :
		_group(group),
		_subgroup(subgroup)
	{}
	
	Message::~Message()
	{}
	
	void MessageObserver::HandleMessage(Message *message)
	{
	}
	
	
	
	void MessageCenter::PostMessage(Message *message)
	{
		Message::MessageGroup group = message->Group();
		Message::MessageSubgroup subgroup = message->Subgroup();
		
		for(auto i=_observer.begin(); i!=_observer.end(); i++)
		{
			if(i->observingGroup == group && (i->observingSubgroup & subgroup))
			{
				i->observer->HandleMessage(message);
			}
		}
	}
	
	
	void MessageCenter::AddObserver(MessageObserver *observer, Message::MessageGroup group)
	{
		AddObserver(observer, group, 0xffffffff);
	}
	
	void MessageCenter::AddObserver(MessageObserver *observer, Message::MessageGroup group, Message::MessageSubgroup subgroup)
	{
		for(auto i=_observer.begin(); i!=_observer.end(); i++)
		{
			while(i->observer == observer && i->observingGroup == group)
			{
				i->observingSubgroup |= subgroup;
				return;
			}
		}
		
		MessageObserverProxy proxy;
		proxy.observer = observer;
		proxy.observingGroup = group;
		proxy.observingSubgroup = subgroup;
		
		_observer.push_back(proxy);
	}
	
	
	void MessageCenter::RemoveObserver(MessageObserver *observer)
	{
		for(auto i=_observer.begin(); i!=_observer.end(); i++)
		{
			while(i->observer == observer)
			{
				i = _observer.erase(i);
				
				if(i == _observer.end())
					return;
			}
		}
	}
	
	void MessageCenter::RemoveObserver(MessageObserver *observer, Message::MessageGroup group)
	{
		for(auto i=_observer.begin(); i!=_observer.end(); i++)
		{			
			if(i->observer == observer && i->observingGroup == group)
			{
				_observer.erase(i);
				break;
			}
		}
	}
	
	void MessageCenter::RemoveObserver(MessageObserver *observer, Message::MessageGroup group, Message::MessageSubgroup subgroup)
	{
		for(auto i=_observer.begin(); i!=_observer.end(); i++)
		{
			if(i->observer == observer && i->observingGroup == group)
			{
				i->observingSubgroup &= ~subgroup;
				
				if(i->observingSubgroup == 0)
				{
					_observer.erase(i);
					break;
				}
			}
		}
	}
}
