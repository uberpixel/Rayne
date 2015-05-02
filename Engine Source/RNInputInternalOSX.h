//
//  RNInputInternalOSX.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INPUTINTERNALOSX_H__
#define __RAYNE_INPUTINTERNALOSX_H__

#include "RNBaseInternal.h"
#include "RNBase.h"
#include "RNInput.h"

#if RN_PLATFORM_MAC_OS

namespace RN
{
	class HIDDevice
	{
	public:		
		HIDDevice(IOHIDDeviceRef device);
		virtual ~HIDDevice();
		
		bool Open();
		bool Close();
		
		String *GetStringProperty(CFStringRef property);
		int32 GetInt32Property(CFStringRef property);
		
		void SendReport(IOHIDReportType type, uint32 reportID, uint8 *report, size_t length);
		
		virtual void HandleInputReport(IOHIDReportType type, uint32 reportID, uint8 *report, size_t length) = 0;
		
	protected:
		IOHIDDeviceRef _device;
		
	private:
		static void InputReportCallback(void *context, IOReturn result, void *sender, IOHIDReportType type, uint32 reportID, uint8 *report, CFIndex reportLength);
		
		uint8 _input[1024];
		bool _connected;
	};
	
	struct HIDElement
	{
		HIDElement(IOHIDElementRef element);
		
		CFIndex logicalMinimum;
		CFIndex logicalMaximum;
		
		CFIndex physicalMinimum;
		CFIndex physicalMaximum;
		
		uint32 usage;
		uint32 usagePage;
		
		size_t reportSize;
		size_t reportCount;
		
		InputControl *control;
	};
	
	class GenericJoystickDevice : public InputDevice, public HIDDevice
	{
	public:
		GenericJoystickDevice(const String *vendor, const String *name, IOHIDDeviceRef device);
		~GenericJoystickDevice();
		
		bool Activate() override;
		bool Deactivate() override;
		void Update() override;
		
		void HandleInputReport(IOHIDReportType type, uint32 reportID, uint8 *report, size_t length) override;
		
	private:
		void ParseHIDElements(IOHIDElementRef parent, CFArrayRef elements);
		
		std::unordered_map<uint32_t, HIDElement *> _elements;
		std::unordered_map<uint32_t, std::vector<HIDElement *>> _reports;
		
		size_t _axisCount;
	};
	
	class Dualshock4Device : public GamepadDevice, public HIDDevice
	{
	public:
		Dualshock4Device(const String *vendor, const String *name, IOHIDDeviceRef device);
		~Dualshock4Device() override;
		
		bool Activate() override;
		bool Deactivate() override;
		void Update() override;
		
		Object *SetRumble(Object *value);
		Object *SetLight(Object *value);
		
		void HandleInputReport(IOHIDReportType type, uint32 reportID, uint8 *report, size_t length) override;

	private:
		void SendReport();
		void Reset();
		
		Axis2DControl *_analogLeft;
		Axis2DControl *_analogRight;
		
		uint8 _rumbleLarge;
		uint8 _rumbleSmall;
		
		uint8 _ledRed;
		uint8 _ledGreen;
		uint8 _ledBlue;
	};
	
	
	void IOKitEnumerateInputDevices();
}

#endif /* RN_PLATFORM_MAC_OS */
#endif /* __RAYNE_INPUTINTERNALOSX_H__ */
