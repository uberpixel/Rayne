//
//  RNInputOSX.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Debug/RNLogger.h"
#include "RNInputOSX.h"

#define kInputQueueSize 512

namespace RN
{
	RNDefineMeta(OSXPlatformDevice, InputDevice)

	const char *kKeyboardButtonName[0xE8] =
		{
			"0x00", "0x01", "0x02", "0x03", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",
			"M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "1", "2",
			"3", "4", "5", "6", "7", "8", "9", "0", "Return", "Escape", "Delete", "Tab", "Space", "-", "=", "[",
			"]", "\\", "Pound", ";", "'", "`", ",", ".", "/", "Caps Lock", "F1", "F2", "F3", "F4", "F5", "F6",
			"F7", "F8", "F9", "F10", "F11", "F12", "Print", "Scroll Lock", "Pause", "Insert", "Home", "Page Up", "Del", "End", "Page Down", "Right",
			"Left", "Down", "Up", "Clear", "Pad /", "Pad *", "Pad -", "Pad +", "Enter", "Pad 1", "Pad 2", "Pad 3", "Pad 4", "Pad 5", "Pad 6", "Pad 7",
			"Pad 8", "Pad 9", "Pad 0", "Pad .", "Slash", "Appl", "Power", "Pad =", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20",
			"F21", "F22", "F23", "F24", "Exec", "Help", "Menu", "Select", "Stop", "Again", "Undo", "Cut", "Copy", "Paste", "Find", "Mute",
			"Vol Up", "Vol Down", "Caps-Lock", "Num-Lock", "Scroll-Lock", "Pad ,", "Pad =", "Inter1", "Inter2", "Inter3", "Inter4", "Inter5", "Inter6", "Inter7", "Inter8", "Inter9",
			"Lang1", "Lang2", "Lang3", "Lang4", "Lang5", "Lang6", "Lang7", "Lang8", "Lang9", "Erase", "SysReq", "Cancel", "Clear", "Prior", "Ret", "Separator",
			"Out", "Oper", "Clear/Again", "CrSel", "ExCel", "0xA5", "0xA6", "0xA7", "0xA8", "0xA9", "0xAA", "0xAB", "0xAC", "0xAD", "0xAE", "0xAF",
			"0xB0", "0xB1", "0xB2", "0xB3", "0xB4", "0xB5", "0xB6", "0xB7", "0xB8", "0xB9", "0xBA", "0xBB", "0xBC", "0xBD", "0xBE", "0xBF",
			"0xC0", "0xC1", "0xC2", "0xC3", "0xC4", "0xC5", "0xC6", "0xC7", "0xC8", "0xC9", "0xCA", "0xCB", "0xCC", "0xCD", "0xCE", "0xCF",
			"0xD0", "0xD1", "0xD2", "0xD3", "0xD4", "0xD5", "0xD6", "0xD7", "0xD8", "0xD9", "0xDA", "0xDB", "0xDC", "0xDD", "0xDE", "0xDF",
			"Left Control", "Left Shift", "Left Option", "Left Command", "Right Control", "Right Shift", "Right Option", "Right Command"
		};

	const char *kOSXDeviceCookieKey = "kOSXDeviceCookieKey";

	OSXPlatformDevice::OSXPlatformDevice(const Descriptor &descriptor, io_object_t object, CFDictionaryRef properties) :
		InputDevice(descriptor),
		_object(object)
	{
		IOObjectRetain(_object);

		SInt32 score;

		if(IOCreatePlugInInterfaceForService(object, kIOHIDDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &_pluginInterface, &score) == kIOReturnSuccess)
		{
			if((**_pluginInterface).QueryInterface(_pluginInterface, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID), (void **)&_deviceInterface) == S_OK)
			{
				(**_deviceInterface).open(_deviceInterface, 0);
				_deviceQueue = (**_deviceInterface).allocQueue(_deviceInterface);

				if(_deviceQueue && ((**_deviceQueue).create(_deviceQueue, 0, kInputQueueSize) == kIOReturnSuccess))
				{
					BuildControlTree(this, properties);
				}
			}
		}
	}

	OSXPlatformDevice::~OSXPlatformDevice()
	{
		if(_deviceQueue)
		{
			(**_deviceQueue).dispose(_deviceQueue);
			(**_deviceQueue).Release(_deviceQueue);
		}

		if(_deviceInterface)
		{
			(**_deviceInterface).close(_deviceInterface);
			(**_deviceInterface).Release(_deviceInterface);
		}

		if(_pluginInterface)
			IODestroyPlugInInterface(_pluginInterface);

		IOObjectRelease(_object);
	}

	void OSXPlatformDevice::BuildControlTree(InputControl *parent, CFDictionaryRef dictionary)
	{
		const void *elementValue = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementKey));

		if(elementValue && CFGetTypeID(elementValue) == CFArrayGetTypeID())
		{
			CFArrayRef elementArray = (CFArrayRef)elementValue;

			CFIndex count = CFArrayGetCount(elementArray);

			for(CFIndex index = 0; index < count; index++)
			{
				const void *value = CFArrayGetValueAtIndex(elementArray, index);
				if(value && CFGetTypeID(value) == CFDictionaryGetTypeID())
				{
					CFDictionaryRef properties = (CFDictionaryRef)value;

					const void *elementTypeValue = CFDictionaryGetValue(properties, CFSTR(kIOHIDElementTypeKey));
					if(elementTypeValue)
					{
						int32 elementType;

						CFNumberGetValue((CFNumberRef)elementTypeValue, kCFNumberSInt32Type, &elementType);
						switch(elementType)
						{
							case kIOHIDElementTypeCollection:
							{
								InputControl *group = new InputControlGroup();
								parent->AddControl(group);

								BuildControlTree(group, properties);
								group->Release();
								break;
							}

							case kIOHIDElementTypeInput_Misc:
							case kIOHIDElementTypeInput_Button:
							case kIOHIDElementTypeInput_Axis:
							case kIOHIDElementTypeInput_ScanCodes:
							{
								const void *cookieValue = CFDictionaryGetValue(properties, CFSTR(kIOHIDElementCookieKey));
								if(cookieValue)
								{
									IOHIDElementCookie cookie;
									CFNumberGetValue((CFNumberRef)cookieValue, kCFNumberSInt32Type, &cookie);

									const void *usagePageValue = CFDictionaryGetValue(properties, CFSTR(kIOHIDElementUsagePageKey));
									const void *usageValue = CFDictionaryGetValue(properties, CFSTR(kIOHIDElementUsageKey));
									if((usagePageValue) && (usageValue))
									{
										int32 usagePage;
										int32 usage;

										CFNumberGetValue((CFNumberRef)usagePageValue, kCFNumberSInt32Type, &usagePage);
										CFNumberGetValue((CFNumberRef)usageValue, kCFNumberSInt32Type, &usage);

										InputControl *control = nullptr;

										switch(usagePage)
										{
											case kHIDPage_KeyboardOrKeypad:
											{
												bool valid = ((uint32)(usage - 0xE0) < 8U);
												if(valid)
												{
													const void *arrayValue = CFDictionaryGetValue(properties, CFSTR(kIOHIDElementIsArrayKey));
													if(arrayValue && CFBooleanGetValue((CFBooleanRef)arrayValue))
														valid = false;
												}
												else
												{
													valid = ((uint32)(usage - 4) < 100U);
												}


												if(valid)
													control = new OSXPlatformButtonControl(RNSTR(kKeyboardButtonName[usage]), cookie);

												break;
											}
										}

										if(control)
										{
											parent->AddControl(control);
											control->Release();
										}
									}
								}

								break;
							}
						}
					}
				}
			}
		}
	}

	void OSXPlatformDevice::Update()
	{
		static const AbsoluteTime zero = {0, 0};
		IOHIDEventStruct event;

		while((**_deviceQueue).getNextEvent(_deviceQueue, &event, zero, 0) == kIOReturnSuccess)
		{
			IOHIDElementCookie cookie = event.elementCookie;

			InputControl *control = GetFirstControl();
			while(control)
			{
				OSXPlatformControl *platformControl = dynamic_cast<OSXPlatformControl *>(control);
				if(platformControl)
				{
					if(platformControl->_cookie == cookie)
					{
						OSXPlatformButtonControl *button = dynamic_cast<OSXPlatformButtonControl *>(control);
						event.value ? button->Start() : button->End();

						break;
					}
				}

				control = control->GetNextControl();
			}
		}
	}

	bool OSXPlatformDevice::__Activate()
	{
		InputControl *control = GetFirstControl();
		while(control)
		{
			OSXPlatformControl *platformControl = dynamic_cast<OSXPlatformControl *>(control);
			if(platformControl)
				(**_deviceQueue).addElement(_deviceQueue, platformControl->_cookie, 0);

			control = control->GetNextControl();
		}

		(**_deviceQueue).start(_deviceQueue);
		return true;
	}
	bool OSXPlatformDevice::__Deactivate()
	{
		(**_deviceQueue).stop(_deviceQueue);

		InputControl *control = GetFirstControl();
		while(control)
		{
			OSXPlatformControl *platformControl = dynamic_cast<OSXPlatformControl *>(control);
			if(platformControl)
				(**_deviceQueue).removeElement(_deviceQueue, platformControl->_cookie);

			control = control->GetNextControl();
		}

		return true;
	}


	RNDefineMeta(OSXPlatformButtonControl, ButtonControl)

	OSXPlatformButtonControl::OSXPlatformButtonControl(const String *name, IOHIDElementCookie cookie) :
		ButtonControl(name),
		OSXPlatformControl(cookie)
	{}


	void BuildPlatformDeviceTree()
	{
		RNInfo("Starting to build device tree");

		io_iterator_t iterator;

		CFMutableDictionaryRef dictionary = IOServiceMatching(kIOHIDDeviceKey);
		if(!dictionary || (IOServiceGetMatchingServices(kIOMasterPortDefault, dictionary, &iterator) != kIOReturnSuccess))
			return;

		while(1)
		{
			CFMutableDictionaryRef properties;

			io_object_t object = IOIteratorNext(iterator);
			if(!object)
				break;

			if(IORegistryEntryCreateCFProperties(object, &properties, kCFAllocatorDefault, kNilOptions) == KERN_SUCCESS)
			{
				CFNumberRef usagePageValue = (CFNumberRef)CFDictionaryGetValue(properties, CFSTR(kIOHIDPrimaryUsagePageKey));
				CFNumberRef usageValue = (CFNumberRef)CFDictionaryGetValue(properties, CFSTR(kIOHIDPrimaryUsageKey));

				if(usagePageValue && usageValue)
				{
					int32 usagePage;
					int32 usage;

					CFNumberGetValue(usagePageValue, kCFNumberSInt32Type, &usagePage);
					CFNumberGetValue(usageValue, kCFNumberSInt32Type, &usage);

					switch(usagePage)
					{
						case kHIDPage_GenericDesktop:
						{
							CFNumberRef productIDValue = (CFNumberRef)CFDictionaryGetValue(properties, CFSTR(kIOHIDVendorIDKey));
							CFNumberRef vendorIDValue = (CFNumberRef)CFDictionaryGetValue(properties, CFSTR(kIOHIDProductIDKey));

							CFStringRef productValue = (CFStringRef)CFDictionaryGetValue(properties, CFSTR(kIOHIDManufacturerKey));
							CFStringRef vendorValue = (CFStringRef)CFDictionaryGetValue(properties, CFSTR(kIOHIDProductKey));

							uint32 productID;
							uint32 vendorID;

							CFNumberGetValue(productIDValue, kCFNumberSInt32Type, &productID);
							CFNumberGetValue(vendorIDValue, kCFNumberSInt32Type, &vendorID);

							String *product;
							String *vendor;

							{
								char buffer[1024];
								CFStringGetCString(productValue, buffer, 1024, kCFStringEncodingUTF8);
								product = new String(buffer, Encoding::UTF8, false);
								product->Autorelease();

								CFStringGetCString(vendorValue, buffer, 1024, kCFStringEncodingUTF8);
								vendor = new String(buffer, Encoding::UTF8, false);
								vendor->Autorelease();
							}

							switch(usage)
							{
								case kHIDUsage_GD_Keyboard:
								case kHIDUsage_GD_Keypad:

									InputDevice::Descriptor descriptor(InputDevice::Category::Keyboard);
									descriptor.SetVendor(vendor);
									descriptor.SetName(product);
									descriptor.SetVendorID(Number::WithUint32(vendorID));
									descriptor.SetProductID(Number::WithUint32(productID));

									OSXPlatformDevice *device = new OSXPlatformDevice(descriptor, object, properties);
									device->Register();
									device->Activate();
									device->Release();

									RNInfo("Added device" << device);

									break;
							}

							break;
						}
					}


					/*if (usagePage == kHIDPage_GenericDesktop)
					{
						if ((usage == kHIDUsage_GD_Pointer) || (usage == kHIDUsage_GD_Mouse)) deviceList.Append(new MouseDevice(object, properties));
						else if ((usage == kHIDUsage_GD_Keyboard) || (usage == kHIDUsage_GD_Keypad)) deviceList.Append(new KeyboardDevice(object, properties));
						else if ((usage == kHIDUsage_GD_Joystick) || (usage == kHIDUsage_GD_GamePad)) deviceList.Append(new JoystickDevice(object, properties));
					}*/
				}

				CFRelease(properties);
			}

			IOObjectRelease(object);
		}

		IOObjectRelease(iterator);

		RNInfo("Built device tree");
	}
}
