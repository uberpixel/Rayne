//
//  RNNotificationManager.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_NOTIFICATIONMANAGER_H_
#define __RAYNE_NOTIFICATIONMANAGER_H_

#include "../Base/RNBase.h"
#include "../Threads/RNLockable.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNString.h"
#include "RNNotification.h"

namespace RN
{
	class NotificationManager
	{
	public:
		friend class Kernel;

		RNAPI static NotificationManager *GetSharedInstance();

		RNAPI void PostNotification(const String *name, Object *object);
		RNAPI void PostNotification(Notification *notification);

		RNAPI void AddSubscriber(const String *name, const std::function<void (Notification *)> &callback, void *token);
		RNAPI void RemoveSubscriber(const String *name, void *token);

	private:
		struct Subscriber
		{
			Subscriber(std::function<void (Notification *)> tcallback, void *ttoken) :
				callback(tcallback),
				token(ttoken)
			{}

			std::function<void (Notification *)> callback;
			void *token;
		};

		NotificationManager();
		~NotificationManager();

		Lockable _lock;
		std::unordered_map<String *, std::vector<Subscriber>, std::hash<Object>, std::equal_to<Object>> _subscribers;
	};
}


#endif /* __RAYNE_NOTIFICATIONMANAGER_H_ */
