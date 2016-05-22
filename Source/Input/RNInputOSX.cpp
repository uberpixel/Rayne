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
			"]", "\\", "Pound", ";", "'", "`", ",", ".", "/", "Caps Lockable", "F1", "F2", "F3", "F4", "F5", "F6",
			"F7", "F8", "F9", "F10", "F11", "F12", "Print", "Scroll Lockable", "Pause", "Insert", "Home", "Page Up", "Del", "End", "Page Down", "Right",
			"Left", "Down", "Up", "Clear", "Pad /", "Pad *", "Pad -", "Pad +", "Enter", "Pad 1", "Pad 2", "Pad 3", "Pad 4", "Pad 5", "Pad 6", "Pad 7",
			"Pad 8", "Pad 9", "Pad 0", "Pad .", "Slash", "Appl", "Power", "Pad =", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20",
			"F21", "F22", "F23", "F24", "Exec", "Help", "Menu", "Select", "Stop", "Again", "Undo", "Cut", "Copy", "Paste", "Find", "Mute",
			"Vol Up", "Vol Down", "Caps-Lockable", "Num-Lockable", "Scroll-Lockable", "Pad ,", "Pad =", "Inter1", "Inter2", "Inter3", "Inter4", "Inter5", "Inter6", "Inter7", "Inter8", "Inter9",
			"Lang1", "Lang2", "Lang3", "Lang4", "Lang5", "Lang6", "Lang7", "Lang8", "Lang9", "Erase", "SysReq", "Cancel", "Clear", "Prior", "Ret", "Separator",
			"Out", "Oper", "Clear/Again", "CrSel", "ExCel", "0xA5", "0xA6", "0xA7", "0xA8", "0xA9", "0xAA", "0xAB", "0xAC", "0xAD", "0xAE", "0xAF",
			"0xB0", "0xB1", "0xB2", "0xB3", "0xB4", "0xB5", "0xB6", "0xB7", "0xB8", "0xB9", "0xBA", "0xBB", "0xBC", "0xBD", "0xBE", "0xBF",
			"0xC0", "0xC1", "0xC2", "0xC3", "0xC4", "0xC5", "0xC6", "0xC7", "0xC8", "0xC9", "0xCA", "0xCB", "0xCC", "0xCD", "0xCE", "0xCF",
			"0xD0", "0xD1", "0xD2", "0xD3", "0xD4", "0xD5", "0xD6", "0xD7", "0xD8", "0xD9", "0xDA", "0xDB", "0xDC", "0xDD", "0xDE", "0xDF",
			"Left Control", "Left Shift", "Left Option", "Left Command", "Right Control", "Right Shift", "Right Option", "Right Command"
		};

	const char *kLinearAxisName[3] =
		{
			"X-Axis ", "Y-Axis ", "Z-Axis "
		};

	const char *kRotationAxisName[3] =
		{
			"RX-Axis ", "RY-Axis ", "RZ-Axis "
		};

	const char *kDeltaAxisName[3] =
		{
			"X-Delta ", "Y-Delta ", "Z-Delta "
		};


	const char *kOSXDeviceCookieKey = "kOSXDeviceCookieKey";


	OSXPlatformDevice::OSXPlatformDevice(const Descriptor &descriptor, IOHIDDeviceRef device) :
		InputDevice(descriptor),
		_device(device),
		_queue(IOHIDQueueCreate(kCFAllocatorDefault, device, 64, kIOHIDOptionsTypeNone)),
		_buttonCount(0),
		_sliderCount(0),
		_deltaAxisCount(0),
		_rotationAxisCount(0),
		_linearAxisCount(0)
	{
		CFRetain(_device);

		CFArrayRef elements = IOHIDDeviceCopyMatchingElements(_device, NULL, kIOHIDOptionsTypeNone);
		BuildControlTree(this, elements);

		CFRelease(elements);

		// Prepare the queue
		for(HIDElement *element : _allElements)
			IOHIDQueueAddElement(_queue, element->element);
	}

	OSXPlatformDevice::~OSXPlatformDevice()
	{
		CFRelease(_device);
		CFRelease(_queue);

		IOHIDDeviceClose(_device, kIOHIDOptionsTypeNone);

		for(HIDElement *element : _allElements)
			delete element;
	}

	void OSXPlatformDevice::BuildControlTree(InputControl *parent, CFArrayRef elements)
	{
		size_t count = CFArrayGetCount(elements);
		for(size_t i = 0; i < count; i ++)
		{
			IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);

			IOHIDElementType type = IOHIDElementGetType(element);
			IOHIDElementCookie cookie = IOHIDElementGetCookie(element);

			if(_elements.find(cookie) != _elements.end())
				continue;

			switch(type)
			{
				case kIOHIDElementTypeCollection:
				{
					InputControl *group = new InputControlGroup();
					parent->AddControl(group);

					BuildControlTree(group, IOHIDElementGetChildren(element));
					group->Release();
					break;
				}

				case kIOHIDElementTypeInput_Misc:
				case kIOHIDElementTypeInput_Button:
				case kIOHIDElementTypeInput_Axis:
				case kIOHIDElementTypeInput_ScanCodes:
				{
					uint32 usage = IOHIDElementGetUsage(element);
					uint32 usagePage = IOHIDElementGetUsagePage(element);

					InputControl *control = nullptr;

					switch(usagePage)
					{
						case kHIDPage_KeyboardOrKeypad:
						{
							bool valid = ((uint32)(usage - 0xE0) < 8U);
							if(valid)
							{
								if(IOHIDElementIsArray(element))
									valid = false;
							}
							else
							{
								valid = ((uint32)(usage - 4) < 100U);
							}


							if(valid)
								control = new ButtonControl(RNSTR(kKeyboardButtonName[usage]), InputControl::Type::KeyButton);

							break;
						}
						case kHIDPage_GenericDesktop:
						{
							switch(usage)
							{
								case kHIDUsage_GD_X:
								case kHIDUsage_GD_Y:
								case kHIDUsage_GD_Z:
								{
									bool relative = IOHIDElementIsRelative(element);
									AxisControl::Axis axis = static_cast<AxisControl::Axis>((usage - kHIDUsage_GD_X) + 1);

									if(relative)
									{
										control = new DeltaAxisControl(RNSTR(kDeltaAxisName[usage - kHIDUsage_GD_X] << (++ _deltaAxisCount)), axis);
									}
									else
									{
										control = new LinearAxisControl(RNSTR(kLinearAxisName[usage - kHIDUsage_GD_X] << (++ _linearAxisCount)), axis);
									}

									break;
								}
								case kHIDUsage_GD_Wheel:
									control = new DeltaAxisControl(RNSTR(kDeltaAxisName[2] << (++ _deltaAxisCount)), AxisControl::Axis::Z);
									break;

								case kHIDUsage_GD_Rx:
								case kHIDUsage_GD_Ry:
								case kHIDUsage_GD_Rz:
								{
									AxisControl::Axis axis = static_cast<AxisControl::Axis>((usage - kHIDUsage_GD_Rx) + 1);
									control = new RotationAxisControl(RNSTR(kRotationAxisName[usage - kHIDUsage_GD_Rx] << (++ _rotationAxisCount)), axis);
									break;
								}

								case kHIDUsage_GD_Slider:
									control = new SliderControl(RNSTR("Slider " << (++ _sliderCount)));
									break;
							}

							break;
						}
						case kHIDPage_Button:
						{
							String *name = RNSTR("Button " << (++ _buttonCount));
							control = new ButtonControl(name, InputControl::Type::Button);
						}
					}

					if(control)
					{
						HIDElement *hidElement = new HIDElement(element, control);

						_allElements.push_back(hidElement);
						_elements.emplace(cookie, hidElement);

						parent->AddControl(control);
						control->Release();

						switch(control->GetType())
						{
							case InputControl::Type::RotationAxis:
							case InputControl::Type::LinearAxis:
							{
								float vmin = (float)IOHIDElementGetPhysicalMin(element);
								float vmax = (float)IOHIDElementGetPhysicalMax(element);

								static_cast<AxisControl *>(control)->SetRange(vmin, vmax, (vmax - vmin) * 0.15f);
								break;
							}

							case InputControl::Type::Slider:
							{
								float vmin = (float)IOHIDElementGetPhysicalMin(element);
								float vmax = (float)IOHIDElementGetPhysicalMax(element);

								static_cast<SliderControl *>(control)->SetRange(vmin, vmax, (vmax - vmin) * 0.0625f);
								break;
							}

							default:
								break;
						}
					}

					break;
				}

				default:
					break;
			}
		}
	}

	void OSXPlatformDevice::Update()
	{
		InputDevice::Update();

		while(1)
		{
			IOHIDValueRef value = IOHIDQueueCopyNextValueWithTimeout(_queue, 0.0);
			if(!value)
				break;

			IOHIDElementRef element = IOHIDValueGetElement(value);
			IOHIDElementCookie cookie = IOHIDElementGetCookie(element);

			auto iterator = _elements.find(cookie);
			if(iterator != _elements.end())
			{
				HIDElement *hidElement = iterator->second;
				hidElement->HandleValue(value);
			}

			CFRelease(value);
		}
	}

	bool OSXPlatformDevice::__Activate()
	{
		if(IOHIDDeviceOpen(_device, kIOHIDOptionsTypeNone) != kIOReturnSuccess)
			return false;

		IOHIDQueueStart(_queue);

		return true;
	}
	bool OSXPlatformDevice::__Deactivate()
	{
		IOHIDQueueStop(_queue);
		IOHIDDeviceClose(_device, kIOHIDOptionsTypeNone);

		return true;
	}


	// HIDElement

	HIDElement::HIDElement(IOHIDElementRef element, InputControl *control) :
		cookie(IOHIDElementGetCookie(element)),
		logicalMinimum(IOHIDElementGetLogicalMin(element)),
		logicalMaximum(IOHIDElementGetLogicalMax(element)),
		physicalMinimum(IOHIDElementGetPhysicalMin(element)),
		physicalMaximum(IOHIDElementGetPhysicalMax(element)),
		usage(IOHIDElementGetUsage(element)),
		usagePage(IOHIDElementGetUsagePage(element)),
		reportSize(IOHIDElementGetReportSize(element)),
		reportCount(IOHIDElementGetReportCount(element)),
		control(nullptr)
	{
		this->element = (IOHIDElementRef)CFRetain(element);
		this->control = control;
	}

	HIDElement::~HIDElement()
	{
		CFRelease(element);
	}

	void HIDElement::HandleValue(IOHIDValueRef value)
	{
		{
			AxisControl *axisControl = control->Downcast<AxisControl>();
			if(axisControl)
				axisControl->SetValue(IOHIDValueGetIntegerValue(value));
		}

		{
			ButtonControl *buttonControl = control->Downcast<ButtonControl>();
			if(buttonControl)
			{
				CFIndex result = IOHIDValueGetIntegerValue(value);
				buttonControl->SetPressed(result > 0);
			}
		}
	}


	// Device management
	static IOHIDManagerRef _hidManager = nullptr;
	static Array *_foundDevices = nullptr;

	void OSXPlatformHIDDeviceAdded(void *context, IOReturn result, void *sender, IOHIDDeviceRef deviceRef)
	{
		CFNumberRef usagePageValue = (CFNumberRef)IOHIDDeviceGetProperty(deviceRef, CFSTR(kIOHIDPrimaryUsagePageKey));
		CFNumberRef usageValue = (CFNumberRef)IOHIDDeviceGetProperty(deviceRef, CFSTR(kIOHIDPrimaryUsageKey));

		if(!usagePageValue || !usageValue)
		{
			IOHIDDeviceClose(deviceRef, kIOHIDOptionsTypeNone);
			return;
		}

		int32 usagePage;
		int32 usage;

		CFNumberGetValue(usagePageValue, kCFNumberSInt32Type, &usagePage);
		CFNumberGetValue(usageValue, kCFNumberSInt32Type, &usage);

		switch(usagePage)
		{
			case kHIDPage_GenericDesktop:
			{
				CFNumberRef productID = (CFNumberRef)IOHIDDeviceGetProperty(deviceRef, CFSTR(kIOHIDProductIDKey));
				CFNumberRef vendorID  = (CFNumberRef)IOHIDDeviceGetProperty(deviceRef, CFSTR(kIOHIDVendorIDKey));

				CFStringRef productValue = (CFStringRef)IOHIDDeviceGetProperty(deviceRef, CFSTR(kIOHIDProductKey));
				CFStringRef vendorValue  = (CFStringRef)IOHIDDeviceGetProperty(deviceRef, CFSTR(kIOHIDManufacturerKey));

				if(!productID || !vendorID || !productValue || !vendorValue)
					break;

				uint32 product;
				uint32 vendor;

				String *productName;
				String *vendorName;

				{
					CFNumberGetValue(productID, kCFNumberSInt32Type, &product);
					CFNumberGetValue(vendorID, kCFNumberSInt32Type, &vendor);
				}
				{
					char name[1024];

					CFStringGetCString(productValue, name, 1024, kCFStringEncodingUTF8);
					productName = RNUTF8STR(name);

					CFStringGetCString(vendorValue, name, 1024, kCFStringEncodingUTF8);
					vendorName = RNUTF8STR(name);
				}


				InputDevice::Category category;
				bool valid = false;

				switch(usage)
				{
					case kHIDUsage_GD_Keyboard:
					case kHIDUsage_GD_Keypad:
					{
						category = InputDevice::Category::Keyboard;
						valid = true;
						break;
					}

					case kHIDUsage_GD_Mouse:
					case kHIDUsage_GD_Pointer:
					{
						category = InputDevice::Category::Mouse;
						valid = true;
						break;
					}
				}

				if(valid)
				{
					RNDebug("Found " << productName << " by " << vendorName);

					InputDevice::Descriptor descriptor(category);
					descriptor.SetVendor(vendorName);
					descriptor.SetName(productName);
					descriptor.SetVendorID(Number::WithUint32(vendor));
					descriptor.SetProductID(Number::WithUint32(product));

					OSXPlatformDevice *device = new OSXPlatformDevice(descriptor, deviceRef);
					device->Register();
					device->Activate();

					_foundDevices->AddObject(device->Autorelease());
				}

				break;
			}
		}
	}
	void OSXPlatformHIDDeviceRemoved(void *context, IOReturn result, void *sender, IOHIDDeviceRef deviceRef)
	{
		_foundDevices->Enumerate<OSXPlatformDevice>([&](OSXPlatformDevice *device, size_t index, bool &stop) {

			if(device->GetRawDevice() == deviceRef)
			{
				if(device->IsActive())
					device->Deactivate();

				if(device->IsRegistered())
					device->Unregister();

				_foundDevices->RemoveObjectAtIndex(index);
				stop = true;
			}

		});
	}


	void BuildPlatformDeviceTree()
	{
		_foundDevices = new Array();
		_hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
		IOHIDManagerSetDeviceMatching(_hidManager, nullptr);

		IOHIDManagerRegisterDeviceMatchingCallback(_hidManager, &OSXPlatformHIDDeviceAdded, nullptr);
		IOHIDManagerRegisterDeviceRemovalCallback(_hidManager, &OSXPlatformHIDDeviceRemoved, nullptr);

		IOHIDManagerScheduleWithRunLoop(_hidManager, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
		IOHIDManagerOpen(_hidManager, kIOHIDOptionsTypeNone);
	}

	void TearDownPlatformDeviceTree()
	{
		_foundDevices->Enumerate<OSXPlatformDevice>([](OSXPlatformDevice *device, size_t index, bool &stop) {

			if(device->IsActive())
				device->Deactivate();

			if(device->IsRegistered())
				device->Unregister();

		});
		_foundDevices->Release();
		_foundDevices = nullptr;

		IOHIDManagerUnscheduleFromRunLoop(_hidManager, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
		IOHIDManagerClose(_hidManager, kIOHIDOptionsTypeNone);

		CFRelease(_hidManager);
		_hidManager = nullptr;
	}
}
