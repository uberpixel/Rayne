//
//  RNHIDDevice.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNHIDDevice.h"
#include "../Objects/RNString.h"

namespace RN
{
	RNDefineMeta(HIDDevice, Object)

	HIDDevice::HIDDevice(HIDUsagePage usagePage, uint16 usage) :
		_usagePage(usagePage),
		_usage(usage)
	{}

	HIDDevice::~HIDDevice()
	{}

	const String *HIDDevice::GetDescription() const
	{
		return RNSTR(Object::GetDescription() << " (" << GetManufacturerString() << ", " << GetProductString() << ")");
	}
}
