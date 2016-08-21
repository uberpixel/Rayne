//
//  RNHIDDevice.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNHIDDevice.h"
#include "RNInputManager.h"
#include "../Objects/RNString.h"

namespace RN
{
	RNDefineMeta(HIDDevice, Object)

	RNExceptionImp(HIDRead)
	RNExceptionImp(HIDWrite)
	RNExceptionImp(HIDOpen)

	HIDDevice::HIDDevice(HIDUsagePage usagePage, uint16 usage) :
		_usagePage(usagePage),
		_usage(usage)
	{}

	HIDDevice::~HIDDevice()
	{}

	const String *HIDDevice::GetDescription() const
	{
		return RNSTR(Object::GetDescription() << " (0x" << std::hex << GetVendorID() << ", 0x" << std::hex << GetProductID() << ", " << GetManufacturerString() << ", " << GetProductString() << ")");
	}

	InputDevice::Descriptor HIDDevice::GetDescriptor() const
	{
		InputDevice::Category category = 0;

		if(GetUsagePage() == HIDUsagePage::GenericDesktop)
		{
			HIDUsageGD usage = GetUsage<HIDUsageGD>();
			switch(usage)
			{
				case HIDUsageGD::Joystick:
					category = InputDevice::Category::Joystick;
					break;
				case HIDUsageGD::GamePad:
					category = InputDevice::Category::Gamepad;
					break;
				default:
					break;
			}
		}

		InputDevice::Descriptor descriptor(category);
		descriptor.SetName(GetProductString());
		descriptor.SetVendor(GetManufacturerString());
		descriptor.SetProductID(Number::WithUint16(GetProductID()));
		descriptor.SetVendorID(Number::WithUint16(GetVendorID()));

		return descriptor;
	}

	void HIDDevice::Register()
	{
		InputManager::GetSharedInstance()->__AddRawHIDDevice(this);
	}
	void HIDDevice::Unregister()
	{
		InputManager::GetSharedInstance()->__RemoveRawHIDDevice(this);
	}
}
