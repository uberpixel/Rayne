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

#if RN_PLATFORM_WINDOWS
#pragma push_macro("PostMessage")
#pragma push_macro("GetObject")
#undef PostMessage
#undef GetObject
#endif

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

#if RN_PLATFORM_WINDOWS
		Object *GetObjectA() const { return _object;  }
		Object *GetObjectW() const { return _object;  }
#endif
		
	protected:
		String *_name;
		Object *_object;
		Dictionary *_info;
		
		RNDeclareMeta(Message)
	};
	
	RNObjectClass(Message)
	
	class MessageCenter : public ISingleton<MessageCenter>
	{
	public:
		typedef std::function<void (Message *)> CallbackType;
		
		RNAPI void PostMessage(Message *message);
		RNAPI void PostMessage(String *name, Object *object, Dictionary *info);

#if RN_PLATFORM_WINDOWS
		// Windows likes its PostMessage macro that expands to either PostMessageA or PostMessageW, depending on
		// wether the user compiles for UniCode or ANSI targets... Not much that we can do about it, except of preach
		// that macros are evil!

		void PostMessageA(Message *message) { PostMessage(message); }
		void PostMessageW(Message *message) { PostMessage(message); }

		void PostMessageA(String *name, Object *object, Dictionary *info) { PostMessage(name, object, info); }
		void PostMessageW(String *name, Object *object, Dictionary *info) { PostMessage(name, object, info); }
#endif
		
		RNAPI void AddObserver(String *name, CallbackType callback, void *cookie);
		
		template<class Function, class Receiver>
		void AddObserver(String *name, Function &&function, Receiver receiver, void *cookie)
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

#if RN_PLATFORM_WINDOWS
#pragma pop_macro("PostMessage")
#pragma pop_macro("GetObject")
#endif

#endif /* __RAYNE_MESSAGE_H__ */
