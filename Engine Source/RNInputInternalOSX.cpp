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
	HIDDevice::HIDDevice(IOHIDDeviceRef device) :
		_device(device),
		_connected(true)
	{
		CFRetain(_device);
		
		IOHIDDeviceScheduleWithRunLoop(_device, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
		IOHIDDeviceRegisterInputReportCallback(_device, _input, 1024, &HIDDevice::InputReportCallback, this);
	}
	
	HIDDevice::~HIDDevice()
	{
		IOHIDDeviceUnscheduleFromRunLoop(_device, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
		IOHIDDeviceRegisterInputReportCallback(_device, _input, 1024, nullptr, this);
		
		IOHIDDeviceClose(_device, kIOHIDOptionsTypeNone);
		CFRelease(_device);
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

	
	
	
	Dualshock4Device::Dualshock4Device(const String *vendor, const String *name, IOHIDDeviceRef device) :
		GamepadDevice(Category::Gamepad, vendor, name),
		HIDDevice(device),
		_active(false),
		_rumbleLarge(0),
		_rumbleSmall(0),
		_ledRed(0),
		_ledBlue(0),
		_ledGreen(0)
	{
		BindCommand(RNUTF8STR("rumble"), std::bind(&Dualshock4Device::SetRumble, this, std::placeholders::_1), { Number::GetMetaClass(), Array::GetMetaClass() });
		BindCommand(RNUTF8STR("light"), std::bind(&Dualshock4Device::SetLight, this, std::placeholders::_1), { Value::GetMetaClass() });
	}
	
	Dualshock4Device::~Dualshock4Device()
	{
		if(_active)
			Deactivate();
	}
	
	
	void Dualshock4Device::Reset()
	{
		_rumbleLarge = 0;
		_rumbleSmall = 0;
		
		_ledRed = _ledGreen = _ledBlue = 0;
		
		SendReport();
	}
	
	
	
	void Dualshock4Device::Activate()
	{
		_active = true;
		Reset();
	}
	
	void Dualshock4Device::Deactivate()
	{
		_active = false;
		Reset();
	}
	
	
	void Dualshock4Device::Update()
	{
		if(!_active)
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
		if(reportID == 0x1)
		{
			Vector2 left(report[1], report[2]);
			Vector2 right(report[3], report[4]);
			
			_analog1 = (left  - 128) / 128;
			_analog2 = (right - 128) / 128;
			
			if(_analog1.GetLength() < 0.05)
				_analog1 = Vector2();
			
			if(_analog2.GetLength() < 0.05)
				_analog2 = Vector2();
		}
	}
	
	
	
	
	
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
		
		CFNumberGetValue(productID, kCFNumberSInt32Type, &product);
		CFNumberGetValue(vendorID, kCFNumberSInt32Type, &vendor);
		
		{
			char name[512];
			
			CFStringGetCString(tproductName, name, 512, CFStringGetSystemEncoding());
			productName = RNUTF8STR(name);
			
			CFStringGetCString(tvendorName, name, 512, CFStringGetSystemEncoding());
			vendorName = RNUTF8STR(name);
		}
		
		
		if(vendor == 1356 && product == 1476)
		{
			Dualshock4Device *device = new Dualshock4Device(vendorName, productName, deviceRef);
			Input::GetSharedInstance()->RegisterDevice(device);
			device->Release();			
		}
		else
		{
			IOHIDDeviceClose(deviceRef, kIOHIDOptionsTypeNone);
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
