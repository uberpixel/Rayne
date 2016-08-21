//
//  RNNotification.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_NOTIFICATION_H_
#define __RAYNE_NOTIFICATION_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNString.h"

namespace RN
{
	class Notification : public Object
	{
	public:
		RNAPI Notification(const String *name, Object *info);
		RNAPI ~Notification();

		const String *GetName() const { return _name; }

		template<class T>
		T *GetInfo() const
		{
			return _info->Downcast<T>();
		}

	private:
		const String *_name;
		Object *_info;
	};
}


#endif /* __RAYNE_NOTIFICATION_H_ */
