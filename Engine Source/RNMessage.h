//
//  RNMessage.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MESSAGE_H__
#define __RAYNE_MESSAGE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNThread.h"

namespace RN
{
	class Message : public Object
	{
	public:
		typedef enum
		{
			MessageGroupInput
		} MessageGroup;
		
		typedef uint32 MessageSubgroup;
		
		MessageGroup Group() const { return _group; }
		MessageSubgroup Subgroup() const { return _subgroup; }
		
	protected:
		Message(MessageGroup group, MessageSubgroup subgroup);
		virtual ~Message();
		
		MessageGroup _group;
		MessageSubgroup _subgroup;
	};
	
	
	
	class MessageObserver
	{
	public:
		virtual void HandleMessage(Message *message);
	};
	
	class MessageCenter : public Singleton<MessageCenter>
	{
	public:
		RNAPI void PostMessage(Message *message);
		
		RNAPI void AddObserver(MessageObserver *observer, Message::MessageGroup group);
		RNAPI void AddObserver(MessageObserver *observer, Message::MessageGroup group, Message::MessageSubgroup subgroup);
		
		RNAPI void RemoveObserver(MessageObserver *observer);
		RNAPI void RemoveObserver(MessageObserver *observer, Message::MessageGroup group);
		RNAPI void RemoveObserver(MessageObserver *observer, Message::MessageGroup group, Message::MessageSubgroup subgroup);
		
	private:		
		struct MessageObserverProxy
		{
			MessageObserver *observer;
			Message::MessageGroup observingGroup;
			Message::MessageSubgroup observingSubgroup;
		};
		
		std::vector<MessageObserverProxy> _observer;
	};
}

#endif /* __RAYNE_MESSAGE_H__ */
