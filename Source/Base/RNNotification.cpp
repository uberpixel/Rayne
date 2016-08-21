//
//  RNNotification.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNNotification.h"

namespace RN
{
	Notification::Notification(const String *name, Object *info) :
		_name(SafeCopy(name)),
		_info(SafeRetain(info))
	{}
	Notification::~Notification()
	{
		SafeRelease(_name);
		SafeRelease(_info);
	}
}
