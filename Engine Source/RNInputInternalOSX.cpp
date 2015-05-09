//
//  RNInputInternalOSX.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNInputInternalOSX.h"
#include "RNNumber.h"
#include "RNValue.h"

#if RN_PLATFORM_MAC_OS

namespace RN
{
	/***************************
	 *
	 * HIDDevice
	 *
	 ***************************/
	
	HIDDevice::HIDDevice(IOHIDDeviceRef device) :
		_device(device),
		_connected(true)
	{
		CFRetain(_device);
	}
	
	HIDDevice::~HIDDevice()
	{
		CFRelease(_device);
	}
	
	bool HIDDevice::Open()
	{
		IOReturn result = IOHIDDeviceOpen(_device, kIOHIDOptionsTypeNone);
		if(result != kIOReturnSuccess)
			return false;
		
		IOHIDDeviceScheduleWithRunLoop(_device, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
		IOHIDDeviceRegisterInputReportCallback(_device, _input, 1024, &HIDDevice::InputReportCallback, this);
		
		return true;
	}
	
	bool HIDDevice::Close()
	{
		IOHIDDeviceUnscheduleFromRunLoop(_device, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
		IOHIDDeviceRegisterInputReportCallback(_device, _input, 1024, nullptr, this);
		
		IOHIDDeviceClose(_device, kIOHIDOptionsTypeNone);
		return true;
	}
	
	
	String *HIDDevice::GetStringProperty(CFStringRef property)
	{
		CFStringRef value = (CFStringRef)IOHIDDeviceGetProperty(_device, property);
		if(!value || CFGetTypeID(value) != CFStringGetTypeID())
			return nullptr;
		
		char string[512];
		CFStringGetCString(value, string, 512, CFStringGetSystemEncoding());
		
		return RNUTF8STR(string);
	}
	
	int32 HIDDevice::GetInt32Property(CFStringRef property)
	{
		CFNumberRef value = (CFNumberRef)IOHIDDeviceGetProperty(_device, property);
		if(!value || CFGetTypeID(value) != CFNumberGetTypeID())
			return 0;
		
		int32 number;
		CFNumberGetValue(value, kCFNumberSInt32Type, &number);
		
		return number;
	}
	
	
	void HIDDevice::SendReport(IOHIDReportType type, uint32 reportID, uint8 *report, size_t length)
	{
		IOHIDDeviceSetReport(_device, type, reportID, report, length);
	}
	
	void HIDDevice::InputReportCallback(void *context, IOReturn result, void *sender, IOHIDReportType type, uint32 reportID, uint8 *report, CFIndex reportLength)
	{
		HIDDevice *device = reinterpret_cast<HIDDevice *>(context);
		device->HandleInputReport(type, reportID, report, reportLength);
	}
	
	/***************************
	 *
	 * HIDElement
	 *
	 ***************************/
	
	HIDElement::HIDElement(IOHIDElementRef element) :
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
	}

	/***************************
	 *
	 * GenericJoystickDevice
	 *
	 ***************************/
	
	GenericJoystickDevice::GenericJoystickDevice(const String *vendor, const String *name, IOHIDDeviceRef device) :
		InputDevice(Category::Joystick, vendor, name),
		_axisCount(0),
		_device(device),
		_queue(IOHIDQueueCreate(kCFAllocatorDefault, device, 50, kIOHIDOptionsTypeNone))
	{
		CFRetain(_device);
		CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, NULL, kIOHIDOptionsTypeNone);

		RNDebug("Device: %s", name->GetUTF8String());
		ParseHIDElements(nullptr, elements);
		
		CFRelease(elements);
	}
	
	GenericJoystickDevice::~GenericJoystickDevice()
	{
	}
	
	void GenericJoystickDevice::ParseHIDElements(IOHIDElementRef parent, CFArrayRef elements)
	{
		size_t count = CFArrayGetCount(elements);
		for(size_t i = 0; i < count; i ++)
		{
			IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);
			
			if(IOHIDElementGetParent(element) != parent)
				continue;
			
			IOHIDElementType type = IOHIDElementGetType(element);
			IOHIDElementCookie cookie = IOHIDElementGetCookie(element);
			
			{
				Log::Loggable loggable;
				
				loggable << "Element type: " << type << " ";
				loggable << "report {" << IOHIDElementGetReportID(element) << ", " << IOHIDElementGetReportCount(element) << " * " << IOHIDElementGetReportSize(element) << "} ";
				loggable << "cookie: " << (uint32)IOHIDElementGetCookie(element) << " ";
				loggable << "usage {" << IOHIDElementGetUsagePage(element) << ", " << IOHIDElementGetUsage(element) << "}";
			}
			
			switch(type)
			{
				case kIOHIDElementTypeCollection:
				{
					CFArrayRef children = IOHIDElementGetChildren(element);
					ParseHIDElements(element, children);
					break;
				}
					
				default:
					break;
			}
			
			if(type < kIOHIDElementTypeOutput)
			{
				auto iterator = _elements.find(cookie);
				if(iterator == _elements.end())
				{
					HIDElement *telement = new HIDElement(element);
					_elements.emplace(cookie, telement);
					
					std::vector<HIDElement *> &telements = _reports[IOHIDElementGetReportID(element)];
					telements.push_back(telement);
					_allElements.push_back(telement);
					
					switch(telement->usagePage)
					{
						case kHIDPage_GenericDesktop:
						{
							InputControl *control = nullptr;
							
							switch(telement->usage)
							{
								case kHIDUsage_GD_X:
									control = new AxisControl(RNSTR("Axis X %d", (int)(_axisCount ++)));
									break;
								case kHIDUsage_GD_Y:
									control = new AxisControl(RNSTR("Axis Y %d", (int)(_axisCount ++)));
									break;
								case kHIDUsage_GD_Z:
									control = new AxisControl(RNSTR("Axis Z %d", (int)(_axisCount ++)));
									break;
									
								case kHIDUsage_GD_Rx:
									control = new AxisControl(RNSTR("Axis Rx %d", (int)(_axisCount ++)));
									break;
								case kHIDUsage_GD_Ry:
									control = new AxisControl(RNSTR("Axis Ry %d", (int)(_axisCount ++)));
									break;
								case kHIDUsage_GD_Rz:
									control = new AxisControl(RNSTR("Axis Rz %d", (int)(_axisCount ++)));
									break;
							}
							
							telement->control = control;
							if(control)
							{
								AddControl(control);
								control->Release();
							}
							break;
						}
							
						case kHIDPage_Button:
						{
							String *name = RNSTR("Button %u", telement->usage);
							ButtonControl *control = new ButtonControl(name);
							
							telement->control = control;
							
							AddControl(control);
							control->Release();
							break;
						}
							
						default:
							break;
					}
				}
			}
		}
		
	}
	
	bool GenericJoystickDevice::Activate()
	{
		if(InputDevice::Activate())
		{
			IOHIDDeviceOpen(_device, kIOHIDOptionsTypeNone);
			IOHIDDeviceScheduleWithRunLoop(_device, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
			
			IOHIDQueueStart(_queue);
			IOHIDQueueRegisterValueAvailableCallback(_queue, &GenericJoystickDevice::QueueCallback, this);
			
			for(HIDElement *element : _allElements)
			{
				IOHIDQueueAddElement(_queue, element->element);
			}
			
			IOHIDQueueScheduleWithRunLoop(_queue, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
			
			return true;
		}
		
		return false;
	}
	
	bool GenericJoystickDevice::Deactivate()
	{
		if(InputDevice::Deactivate())
		{
			IOHIDQueueStop(_queue);
			return true;
		}
		
		return false;
	}
	
	void GenericJoystickDevice::Update()
	{}
	
	void GenericJoystickDevice::HandleQueue()
	{
		while(1)
		{
			IOHIDValueRef value = IOHIDQueueCopyNextValueWithTimeout(_queue, 0.0);
			if(!value)
				break;
			
			IOHIDElementRef element = IOHIDValueGetElement(value);
			IOHIDElementCookie cookie = IOHIDElementGetCookie(element);
			
			for(HIDElement *helement : _allElements)
			{
				if(helement->cookie == cookie)
				{
					switch(helement->usagePage)
					{
						case kHIDPage_GenericDesktop:
						{
							switch(helement->usage)
							{
								case kHIDUsage_GD_X:
								case kHIDUsage_GD_Y:
								case kHIDUsage_GD_Z:
								case kHIDUsage_GD_Rx:
								case kHIDUsage_GD_Ry:
								case kHIDUsage_GD_Rz:
								{
									AxisControl *control = static_cast<AxisControl *>(helement->control);
									double result = static_cast<double>((IOHIDValueGetScaledValue(value, kIOHIDValueScaleTypePhysical) + helement->logicalMinimum)) / helement->logicalMaximum;
									
									result = (result * 2.0) - 1.0;
									
									control->SetValue(result);
									
									break;
								}
							}
							break;
						}
							
						case kHIDPage_Button:
						{
							CFIndex result = IOHIDValueGetIntegerValue(value);
							
							ButtonControl *control = static_cast<ButtonControl *>(helement->control);
							control->SetPressed(result > 0);
							break;
						}

					}
				}
			}
			
			CFRelease(value);
		}
	}
	
	void GenericJoystickDevice::QueueCallback(void *context, IOReturn result, void *sender)
	{
		GenericJoystickDevice *device = reinterpret_cast<GenericJoystickDevice *>(context);
		device->HandleQueue();
	}

	/***************************
	 *
	 * Dualshock4Device
	 *
	 ***************************/
	
	Dualshock4Device::Dualshock4Device(const String *vendor, const String *name, IOHIDDeviceRef device) :
		GamepadDevice(Category::Gamepad, vendor, name),
		HIDDevice(device),
		_rumbleLarge(0),
		_rumbleSmall(0),
		_ledRed(0),
		_ledBlue(0),
		_ledGreen(0)
	{
		BindCommand(RNUTF8STR("rumble"), std::bind(&Dualshock4Device::SetRumble, this, std::placeholders::_1), { Number::GetMetaClass(), Array::GetMetaClass() });
		BindCommand(RNUTF8STR("light"), std::bind(&Dualshock4Device::SetLight, this, std::placeholders::_1), { Value::GetMetaClass() });
	
	
		_analogLeft  = new Axis2DControl(RNCSTR("Left Analog"));
		_analogRight = new Axis2DControl(RNCSTR("Right Analog"));
		
		_analogLeft->SetDeadzone(0.05);
		_analogRight->SetDeadzone(0.05);
		
		AddControl(_analogLeft);
		AddControl(_analogRight);
	}
	
	Dualshock4Device::~Dualshock4Device()
	{
		if(IsActive())
			Deactivate();
		
		_analogLeft->Release();
		_analogRight->Release();
	}
	
	
	void Dualshock4Device::Reset()
	{
		_rumbleLarge = 0;
		_rumbleSmall = 0;
		
		_ledRed = _ledGreen = _ledBlue = 0;
		
		SendReport();
	}
	
	
	
	bool Dualshock4Device::Activate()
	{
		if(GamepadDevice::Activate())
		{
			if(!Open())
			{
				GamepadDevice::Deactivate();
				return false;
			}
			
			Reset();
			return true;
		}
		
		return false;
	}
	
	bool Dualshock4Device::Deactivate()
	{
		if(GamepadDevice::Deactivate())
		{
			Close();
			Reset();
			
			return true;
		}
		
		return false;
	}
	
	
	void Dualshock4Device::Update()
	{
		if(!IsActive())
			return;
		
		SendReport();
	}
	
	
	Object *Dualshock4Device::SetRumble(Object *value)
	{
		if(value->IsKindOfClass(Number::GetMetaClass()))
		{
			Number *number = value->Downcast<Number>();
			_rumbleLarge = _rumbleSmall = number->GetUint8Value();
		}
		
		if(value->IsKindOfClass(Array::GetMetaClass()))
		{
			Array *array = value->Downcast<Array>();
			
			if(array->GetCount() != 2)
				throw Exception(Exception::Type::InvalidArgumentException, "Array must have two entries (RN::Number)");
			
			Number *large = array->GetObjectAtIndex<Number>(0);
			Number *small = array->GetObjectAtIndex<Number>(1);
			
			_rumbleLarge = large->GetUint8Value();
			_rumbleSmall = small->GetUint8Value();
		}
		
		return nullptr;
	}
	
	Object *Dualshock4Device::SetLight(Object *value)
	{
		Value *object = value->Downcast<Value>();
		if(object && object->CanConvertToType<Vector3>())
		{
			Vector3 vector = object->GetValue<Vector3>();
			vector.Normalize();
			
			_ledRed   = static_cast<uint8>(vector.x * 255);
			_ledGreen = static_cast<uint8>(vector.y * 255);
			_ledBlue  = static_cast<uint8>(vector.z * 255);
		}
		
		
		return nullptr;
	}
	
	
	void Dualshock4Device::SendReport()
	{
		uint8 report[32];
		memset(report, 0x0, 32);
		
		report[0] = 0x05;
		report[1] = 0xff;
		
		report[4] = _rumbleLarge;
		report[5] = _rumbleSmall;
		
		report[6] = _ledRed;
		report[7] = _ledGreen;
		report[8] = _ledBlue;
		
		HIDDevice::SendReport(kIOHIDReportTypeOutput, 0x05, report, 32);
	}
	
	
	void Dualshock4Device::HandleInputReport(IOHIDReportType type, uint32 reportID, uint8 *report, size_t length)
	{
		RN_ASSERT(IsActive(), "HandleInputReport() called on deactived device!");
		
		if(reportID == 0x1)
		{
			// Analog sticks
			Vector2 left(report[1], report[2]);
			Vector2 right(report[3], report[4]);
			
			_analog1 = (left  - 128) / 128;
			_analog2 = (right - 128) / 128;
			
			if(_analog1.GetLength() < _analogLeft->GetDeadzone())
				_analog1 = Vector2();
			
			if(_analog2.GetLength() < _analogRight->GetDeadzone())
				_analog2 = Vector2();
			
			_analogLeft->SetValue(_analog1);
			_analogRight->SetValue(_analog2);
			
			// Buttons
			uint8 dpad = (report[5] & 0xf);
			
			uint8 buttons1 = report[5];
			uint8 buttons2 = report[6];
			
			static uint8 dpadlookup[] = {
				(1 << 0),
				(1 << 0) | (1 << 1),
				(1 << 1),
				(1 << 1) | (1 << 2),
				(1 << 2),
				(1 << 2) | (1 << 3),
				(1 << 3),
				(1 << 3) | (1 << 0),
				0
			};
			
			_buttons = dpadlookup[dpad];
			
			_buttons |= (buttons1 & (1 << 7)) ? (1 << 4) : 0;
			_buttons |= (buttons1 & (1 << 6)) ? (1 << 5) : 0;
			_buttons |= (buttons1 & (1 << 5)) ? (1 << 6) : 0;
			_buttons |= (buttons1 & (1 << 4)) ? (1 << 7) : 0;
			
			_buttons |= (buttons2 & (1 << 0)) ? (1 << 8) : 0;
			_buttons |= (buttons2 & (1 << 1)) ? (1 << 9) : 0;
			_buttons |= (buttons2 & (1 << 2)) ? (1 << 10) : 0;
			_buttons |= (buttons2 & (1 << 3)) ? (1 << 11) : 0;
			_buttons |= (buttons2 & (1 << 4)) ? (1 << 12) : 0;
			_buttons |= (buttons2 & (1 << 5)) ? (1 << 13) : 0;
			_buttons |= (buttons2 & (1 << 6)) ? (1 << 14) : 0;
			_buttons |= (buttons2 & (1 << 7)) ? (1 << 15) : 0;
			
			// Trigger
			_trigger1 = (report[8] / 255.0);
			_trigger2 = (report[9] / 255.0);
		}
	}
	
	
	
	/***************************
	 *
	 * Device manager
	 *
	 ***************************/
	
	void IOKitHIDDeviceAdded(void *context, IOReturn result, void *sender, IOHIDDeviceRef deviceRef)
	{
		result = IOHIDDeviceOpen(deviceRef, kIOHIDOptionsTypeNone);
		if(result != kIOReturnSuccess)
			return;
		
		CFNumberRef productID = (CFNumberRef)IOHIDDeviceGetProperty(deviceRef, CFSTR(kIOHIDProductIDKey));
		CFNumberRef vendorID  = (CFNumberRef)IOHIDDeviceGetProperty(deviceRef, CFSTR(kIOHIDVendorIDKey));
		
		CFStringRef tproductName = (CFStringRef)IOHIDDeviceGetProperty(deviceRef, CFSTR(kIOHIDProductKey));
		CFStringRef tvendorName  = (CFStringRef)IOHIDDeviceGetProperty(deviceRef, CFSTR(kIOHIDManufacturerKey));
		
		if(!productID || !vendorID || !tproductName || !tvendorName)
		{
			IOHIDDeviceClose(deviceRef, kIOHIDOptionsTypeNone);
			return;
		}
		
		int32 product;
		int32 vendor;
		
		String *productName;
		String *vendorName;
		
		{
			CFNumberGetValue(productID, kCFNumberSInt32Type, &product);
			CFNumberGetValue(vendorID, kCFNumberSInt32Type, &vendor);
		}
		{
			char name[512];
			
			CFStringGetCString(tproductName, name, 512, CFStringGetSystemEncoding());
			productName = RNUTF8STR(name);
			
			CFStringGetCString(tvendorName, name, 512, CFStringGetSystemEncoding());
			vendorName = RNUTF8STR(name);
		}
		
		InputDevice *device = nullptr;
		
		// Match specialized devices
		if(vendor == 0x54c && product == 0x5c4)
			device = new Dualshock4Device(vendorName, productName, deviceRef); // Playstation 4 controller
		
		// Generic devices
		if(IOHIDDeviceConformsTo(deviceRef, kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick))
		{
			device = new GenericJoystickDevice(vendorName, productName, deviceRef);
		}
		
		IOHIDDeviceClose(deviceRef, kIOHIDOptionsTypeNone);
		
		if(device)
		{
			Input::GetSharedInstance()->RegisterDevice(device);
			device->Release();
		}
	}
	
	
	
	static IOHIDManagerRef __hidManager = nullptr;
	
	void IOKitEnumerateInputDevices()
	{
		__hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
		IOHIDManagerSetDeviceMatching(__hidManager, nullptr);
		
		IOHIDManagerRegisterDeviceMatchingCallback(__hidManager, IOKitHIDDeviceAdded, nullptr);
		
		IOHIDManagerScheduleWithRunLoop(__hidManager, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
		IOHIDManagerOpen(__hidManager, kIOHIDOptionsTypeNone);
	}
}

#endif /* RN_PLATFORM_MAC_OS */
