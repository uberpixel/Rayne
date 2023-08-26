//
//  RNNotificationManager.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNNotificationManager.h"

namespace RN
{
	static NotificationManager *_sharedInstance;

	NotificationManager::NotificationManager()
	{
		_sharedInstance = this;
	}
	NotificationManager::~NotificationManager()
	{
		_sharedInstance = nullptr;
		
		//TODO: Should enumerate _subscribers here and release "name"
	}

	NotificationManager *NotificationManager::GetSharedInstance()
	{
		return _sharedInstance;
	}


	void NotificationManager::PostNotification(const String *name, Object *object)
	{
		Notification *notification = new Notification(name, object);
		PostNotification(notification);
		notification->Release();
	}
	void NotificationManager::PostNotification(Notification *notification)
	{
		UniqueLock<Lockable>  lock(_lock);

		auto iterator = _subscribers.find(const_cast<String *>(notification->GetName()));
		if(iterator == _subscribers.end())
			return;

		std::vector<Subscriber> copy = iterator->second;
		lock.Unlock();

		for(auto &subscriber : copy)
			subscriber.callback(notification);
	}


	void NotificationManager::AddSubscriber(const String *name, const std::function<void (Notification *)> &callback, void *token)
	{
		LockGuard<Lockable> lock(_lock);

		auto iterator = _subscribers.find(const_cast<String *>(name));
		if(iterator == _subscribers.end())
		{
			String *key = name->Copy(); //Copy retains it, released when removing the subscriber

			std::vector<Subscriber> subscribers;
			subscribers.emplace_back(callback, token);

			_subscribers.emplace(std::make_pair(key, std::move(subscribers)));
		}
	}

	void NotificationManager::RemoveSubscriber(const String *name, void *token)
	{
		LockGuard<Lockable> lock(_lock);

		auto iterator = _subscribers.find(const_cast<String *>(name));
		if(iterator == _subscribers.end())
			return;

		for(auto it = iterator->second.begin(); it != iterator->second.end();)
		{
			if(it->token == token)
			{
				it = iterator->second.erase(it);
				continue;
			}

			it ++;
		}
		
		iterator->first->Release(); //Release the "name"
	}
}
