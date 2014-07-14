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
#include "RNInput.h"

#if RN_PLATFORM_MAC_OS

namespace RN
{
	class HIDDevice
	{
	public:		
		HIDDevice(IOHIDDeviceRef device);
		virtual ~HIDDevice();
		
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
	
	class Dualshock4Device : public GamepadDevice, public HIDDevice
	{
	public:
		Dualshock4Device(const String *vendor, const String *name, IOHIDDeviceRef device);
		~Dualshock4Device() override;
		
		void Activate() override;
		void Deactivate() override;
		void Update() override;
		
		Object *SetRumble(Object *value);
		Object *SetLight(Object *value);
		
		void HandleInputReport(IOHIDReportType type, uint32 reportID, uint8 *report, size_t length) override;

	private:
		void SendReport();
		void Reset();
		
		bool _active;
		
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
