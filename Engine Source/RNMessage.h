//
//  RNMessage.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MESSAGE_H__
#define __RAYNE_MESSAGE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNString.h"
#include "RNDictionary.h"
#include "RNThread.h"

namespace RN
{
	class Message : public Object
	{
	public:
		RNAPI Message(String *name, Object *object, Dictionary *info);
		RNAPI ~Message() override;
		
		RNAPI String *GetName() const { return _name; }
		RNAPI Object *GetObject() const { return _object; }
		RNAPI Dictionary *GetInfo() const { return _info; }
		
	protected:
		String *_name;
		Object *_object;
		Dictionary *_info;
		
		RNDeclareMeta(Message)
	};
	
	class MessageCenter : public ISingleton<MessageCenter>
	{
	public:
		typedef std::function<void (Message *)> CallbackType;
		
		RNAPI void PostMessage(Message *message);
		RNAPI void PostMessage(String *name, Object *object, Dictionary *info);
		
		RNAPI void AddObserver(String *name, CallbackType callback, void *cookie);
		
		template<class Function, class Receiver>
		void AddObserver(String *name, Function&& function, Receiver receiver, void *cookie)
		{
			AddObserver(name, std::bind(function, receiver, std::placeholders::_1), cookie);
		}
		
		RNAPI void RemoveObserver(void *cookie);
		RNAPI void RemoveObserver(void *cookie, String *name);
		
	private:		
		struct MessageObserverProxy
		{
			CallbackType callback;
			void *cookie;
		};
		
		SpinLock _lock;
		std::unordered_map<String *, std::vector<MessageObserverProxy>, std::hash<Object>, std::equal_to<Object>> _observer;
		
		RNDeclareSingleton(MessageCenter)
	};
}

#endif /* __RAYNE_MESSAGE_H__ */
