//
//  RNGamepad.cpp
//  Rayne
//
//  Created by Nils Daumann on 10.08.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "RNGamepad.h"

namespace RN
{
	GamepadManager::GamepadManager()
	{
		gamepad = new Gamepad();
		
		_hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
		IOHIDManagerOpen(_hidManager, kIOHIDOptionsTypeNone);
		IOHIDManagerScheduleWithRunLoop(_hidManager,
										CFRunLoopGetCurrent(),
										kCFRunLoopDefaultMode);
		
		
		// Setup device matching.
		CFStringRef keys[] = {  CFSTR(kIOHIDDeviceUsagePageKey),
			CFSTR(kIOHIDDeviceUsageKey)};
		
		int value;
		CFNumberRef values[2];
		CFDictionaryRef dictionaries[2];
		
		// Match joysticks.
		value = kHIDPage_GenericDesktop;
		values[0] = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &value);
		
		value = kHIDUsage_GD_Joystick;
		values[1] = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &value);
		
		dictionaries[0] = CFDictionaryCreate(kCFAllocatorDefault,
											 (const void **) keys,
											 (const void **) values,
											 2,
											 &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		CFRelease(values[0]);
		CFRelease(values[1]);
		
		// Match gamepads.
		value = kHIDPage_GenericDesktop;
		values[0] = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &value);
		
		value = kHIDUsage_GD_GamePad;
		values[1] = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &value);
		
		dictionaries[1] = CFDictionaryCreate(kCFAllocatorDefault,
											 (const void **) keys,
											 (const void **) values,
											 2,
											 &kCFTypeDictionaryKeyCallBacks,
											 &kCFTypeDictionaryValueCallBacks);
		CFRelease(values[0]);
		CFRelease(values[1]);
		
		CFArrayRef array = CFArrayCreate(   kCFAllocatorDefault,
										 (const void **) dictionaries,
										 2,
										 &kCFTypeArrayCallBacks);
		CFRelease(dictionaries[0]);
		CFRelease(dictionaries[1]);
		
		IOHIDManagerSetDeviceMatchingMultiple(_hidManager, array);
		
		CFRelease(array);
		
		
		IOHIDManagerRegisterDeviceMatchingCallback(_hidManager, OnDeviceConnected, this);
		IOHIDManagerRegisterDeviceRemovalCallback(_hidManager, OnDeviceRemoved, this);
	}
	
	int GamepadManager::GetIntDeviceProperty(IOHIDDeviceRef device, CFStringRef key)
	{
		CFTypeRef type = IOHIDDeviceGetProperty(device, key);
		RN_ASSERT(type != NULL && CFGetTypeID(type) == CFNumberGetTypeID(), "");
		
		int value;
		CFNumberGetValue((CFNumberRef) type, kCFNumberSInt32Type, &value);
		
		return value;
	}
	
	float GamepadManager::MapAnalogAxis(IOHIDValueRef value, IOHIDElementRef element)
	{
		
		CFIndex val = IOHIDValueGetIntegerValue(value);
		CFIndex min = IOHIDElementGetLogicalMin(element);
		CFIndex max = IOHIDElementGetLogicalMax(element);
		
		float v = (float) (val - min) / (float) (max - min);
		v = v * 2.0f - 1.0f;
		
		// Dead zone.
		if (v < 0.1f && v > -0.1f)
		{
			v = 0.0f;
		}
		
		return v;
	}
	
	void GamepadManager::OnDeviceConnected(void* context, IOReturn result, void* sender, IOHIDDeviceRef device)
	{
		GamepadManager *manager = GamepadManager::SharedInstance();
		IOHIDDeviceRegisterInputValueCallback(device, OnDeviceValueChanged, manager);
	}
	
	void GamepadManager::OnDeviceRemoved(void* context, IOReturn result, void* sender, IOHIDDeviceRef device)
	{
		IOHIDDeviceRegisterInputValueCallback(device, NULL, NULL);
	}
	
	void GamepadManager::OnDeviceValueChanged(void* context, IOReturn result, void* sender, IOHIDValueRef value)
	{
		IOHIDElementRef element = IOHIDValueGetElement(value);
		IOHIDDeviceRef device = IOHIDElementGetDevice(element);
		
		GamepadManager *manager = static_cast<GamepadManager*>(context);
		
		int vendorID = manager->GetIntDeviceProperty(device, CFSTR(kIOHIDVendorIDKey));
		//int productID = manager->GetIntDeviceProperty(device, CFSTR(kIOHIDProductIDKey));
		
		uint32_t usagePage = IOHIDElementGetUsagePage(element);
		uint32_t usage = IOHIDElementGetUsage(element);
		
		
		// The following controller mapping is based on the Logitech F710, however we use it for
		// all Logitech devices on the assumption that they're likely to share the same mapping.
		if (vendorID == 0x046D)
		{
			// Logitech F710 mapping.
			if (usagePage == kHIDPage_Button)
			{
				bool buttonState = IOHIDValueGetIntegerValue(value);
				
				switch(usage)
				{
					case kHIDUsage_Button_1: //X
						manager->gamepad->SetButton(Gamepad::B1, buttonState);
						break;
					case kHIDUsage_Button_2: //A
						manager->gamepad->SetButton(Gamepad::B2, buttonState);
						break;
					case kHIDUsage_Button_3: //B
						manager->gamepad->SetButton(Gamepad::B3, buttonState);
						break;
					case kHIDUsage_Button_4: //Y
						manager->gamepad->SetButton(Gamepad::B4, buttonState);
						break;
					case 0x05: //L1
						manager->gamepad->SetButton(Gamepad::L1, buttonState);
						break;
					case 0x06: //R1
						manager->gamepad->SetButton(Gamepad::R1, buttonState);
						break;
					case 0x07: //LT
						manager->gamepad->trigger0 = buttonState ? 1.0f:0.0f;
						break;
					case 0x08: //RT
						manager->gamepad->trigger1 = buttonState ? 1.0f:0.0f;
						break;
					case 0x09: //Back
						manager->gamepad->SetButton(Gamepad::Back, buttonState);
						break;
					case 0x0A: //Start
						manager->gamepad->SetButton(Gamepad::Start, buttonState);
						break;
					case 0x0B: //LStick
						manager->gamepad->SetButton(Gamepad::LStick, buttonState);
						break;
					case 0x0C: //RStick
						manager->gamepad->SetButton(Gamepad::RStick, buttonState);
						break;
					default:
						return;
				}
			}
			else if (usagePage == kHIDPage_GenericDesktop)
			{
				float v;
				switch(usage)
				{
					case kHIDUsage_GD_X: //LX
						v = manager->MapAnalogAxis(value, element);
						manager->gamepad->axis0.x = v;
						break;
					case kHIDUsage_GD_Y: //LY
						v = manager->MapAnalogAxis(value, element);
						manager->gamepad->axis0.y = -v;
						break;
					case kHIDUsage_GD_Z: //RX
						v = manager->MapAnalogAxis(value, element);
						manager->gamepad->axis1.x = v;
						break;
					case kHIDUsage_GD_Rz: //RY
						v = manager->MapAnalogAxis(value, element);
						manager->gamepad->axis1.y = -v;
						break;
					case kHIDUsage_GD_Hatswitch:
                    {
                        CFIndex integerValue = IOHIDValueGetIntegerValue(value);
						
                        manager->gamepad->SetButton(Gamepad::Up, //Up
                                           integerValue == 7 || integerValue == 0 || integerValue == 1);
                        manager->gamepad->SetButton(Gamepad::Down, //Down
                                           integerValue == 3 || integerValue == 4 || integerValue == 5);
                        manager->gamepad->SetButton(Gamepad::Left, //Left
                                           integerValue == 5 || integerValue == 6 || integerValue == 7);
                        manager->gamepad->SetButton(Gamepad::Right, //Right
                                           integerValue == 1 || integerValue == 2 || integerValue == 3);
                    }
						break;
					default:
						return;
				}
			}
		}
		// The following controller mapping is based on the Sony DualShock3, however we use it for
		// all Sony devices on the assumption that they're likely to share the same mapping.
		else if (vendorID == 0x054C)
		{
			// PS3 Controller.
			if (usagePage == kHIDPage_Button)
			{
				bool buttonState = IOHIDValueGetIntegerValue(value);
				
				switch(usage)
				{
					case kHIDUsage_Button_1:
						manager->gamepad->SetButton(Gamepad::Back, buttonState);
						break;
					case kHIDUsage_Button_2:
						manager->gamepad->SetButton(Gamepad::LStick, buttonState);
						break;
					case kHIDUsage_Button_3:
						manager->gamepad->SetButton(Gamepad::RStick, buttonState);
						break;
					case kHIDUsage_Button_4:
						manager->gamepad->SetButton(Gamepad::Start, buttonState);
						break;
					case 0x05:
						manager->gamepad->SetButton(Gamepad::Up, buttonState);
						break;
					case 0x06:
						manager->gamepad->SetButton(Gamepad::Right, buttonState);
						break;
					case 0x07:
						manager->gamepad->SetButton(Gamepad::Down, buttonState);
						break;
					case 0x08:
						manager->gamepad->SetButton(Gamepad::Left, buttonState);
						break;
					case 0x09:
						manager->gamepad->trigger0 = buttonState ? 1.0f:0.0f;
						break;
					case 0x0A:
						manager->gamepad->trigger1 = buttonState ? 1.0f:0.0f;
						break;
					case 0x0B:
						manager->gamepad->SetButton(Gamepad::L1, buttonState);
						break;
					case 0x0C:
						manager->gamepad->SetButton(Gamepad::R1, buttonState);
						break;
					case 0x0D:
						// PS3 Triangle.
						manager->gamepad->SetButton(Gamepad::B1, buttonState);
						break;
					case 0x0E:
						// PS3 Circle
						manager->gamepad->SetButton(Gamepad::B2, buttonState);
						break;
					case 0x0F:
						// PS3 Cross
						manager->gamepad->SetButton(Gamepad::B3, buttonState);
						break;
					case 0x10:
						// PS3 Square
						manager->gamepad->SetButton(Gamepad::B4, buttonState);
						break;
					default:
						return;
				}
			}
			else if (usagePage == kHIDPage_GenericDesktop)
			{
				float v;
				switch(usage)
				{
					case kHIDUsage_GD_X:
						v = manager->MapAnalogAxis(value, element);
						manager->gamepad->axis0.x = v;
						break;
					case kHIDUsage_GD_Y:
						v = manager->MapAnalogAxis(value, element);
						manager->gamepad->axis0.y = -v;
						break;
					case kHIDUsage_GD_Z:
						v = manager->MapAnalogAxis(value, element);
						manager->gamepad->axis1.x = v;
						break;
					case kHIDUsage_GD_Rz:
						v = manager->MapAnalogAxis(value, element);
						manager->gamepad->axis1.y = -v;
						break;
					default:
						return;
				}
			}
		}
	}
	
	GamepadManager::~GamepadManager()
	{
		delete gamepad;
	}
}