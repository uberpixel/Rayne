//
//  RNHID.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_HID_H_
#define __RAYNE_HID_H_

#include "../Base/RNBase.h"

// References:
// http://www.usb.org/developers/hidpage
// http://www.usb.org/developers/hidpage/Hut1_12v2.pdf

namespace RN
{
	enum class HIDUsagePage : uint16
	{
		Undefined = 0x0,
		GenericDesktop,
		Simulation,
		VR,
		Sport,
		Game,
		GenericDeviceControls,
		KeyboardOrKeypad,
		LEDs,
		Button,
		Ordinal,
		Telephony,
		Consumer,
		Digitizer,

		PID = 0xf,
		Unicode,

		AlphanumericDisplay = 0x14,

		Sensor = 0x20,

		Monitor = 0x80,
		MonitorEnumerated,
		MonitorVirtual,
		MonitorReserved,

		PowerDevice = 0x84,
		BatterySystem,
		PowerReserved,
		PowerReserved2,

		BarCodeScanner = 0x8c,
		WeighingDevice,
		Scale,
		MangeticStripReader,

		CameraControl = 0x90,
		Arcade,

		VendorDefinedStart = 0xff00
	};

	enum class HIDUsageGD : uint16
	{
		Pointer = 0x01,    /* Physical Collection */
		Mouse = 0x02,    /* Application Collection */
		/* 0x03 Reserved */
		Joystick = 0x04,    /* Application Collection */
		GamePad = 0x05,    /* Application Collection */
		Keyboard = 0x06,    /* Application Collection */
		Keypad = 0x07,    /* Application Collection */
		MultiAxisController = 0x08,    /* Application Collection */
		TabletPCSystemControls = 0x09,    /* Application Collection */
		AssistiveControl = 0x0A,    /* Application Collection */
		/* 0x0B - 0x2F Reserved */
		X = 0x30,    /* Dynamic Value */
		Y = 0x31,    /* Dynamic Value */
		Z = 0x32,    /* Dynamic Value */
		Rx = 0x33,    /* Dynamic Value */
		Ry = 0x34,    /* Dynamic Value */
		Rz = 0x35,    /* Dynamic Value */
		Slider = 0x36,    /* Dynamic Value */
		Dial = 0x37,    /* Dynamic Value */
		Wheel = 0x38,    /* Dynamic Value */
		Hatswitch = 0x39,    /* Dynamic Value */
		CountedBuffer = 0x3A,    /* Logical Collection */
		ByteCount = 0x3B,    /* Dynamic Value */
		MotionWakeup = 0x3C,    /* One-Shot Control */
		Start = 0x3D,    /* On/Off Control */
		Select = 0x3E,    /* On/Off Control */
		/* 0x3F Reserved */
		Vx = 0x40,    /* Dynamic Value */
		Vy = 0x41,    /* Dynamic Value */
		Vz = 0x42,    /* Dynamic Value */
		Vbrx = 0x43,    /* Dynamic Value */
		Vbry = 0x44,    /* Dynamic Value */
		Vbrz = 0x45,    /* Dynamic Value */
		Vno = 0x46,    /* Dynamic Value */
		/* 0x47 - 0x7F Reserved */
		SystemControl = 0x80,    /* Application Collection */
		SystemPowerDown = 0x81,    /* One-Shot Control */
		SystemSleep = 0x82,    /* One-Shot Control */
		SystemWakeUp = 0x83,    /* One-Shot Control */
		SystemContextMenu = 0x84,    /* One-Shot Control */
		SystemMainMenu = 0x85,    /* One-Shot Control */
		SystemAppMenu = 0x86,    /* One-Shot Control */
		SystemMenuHelp = 0x87,    /* One-Shot Control */
		SystemMenuExit = 0x88,    /* One-Shot Control */
		SystemMenuSelect = 0x89,    /* Selector */
		SystemMenu = 0x89,    /* Selector */
		SystemMenuRight = 0x8A,    /* Re-Trigger Control */
		SystemMenuLeft = 0x8B,    /* Re-Trigger Control */
		SystemMenuUp = 0x8C,    /* Re-Trigger Control */
		SystemMenuDown = 0x8D,    /* Re-Trigger Control */
		/* 0x8E - 0x8F Reserved */
		DPadUp = 0x90,    /* On/Off Control */
		DPadDown = 0x91,    /* On/Off Control */
		DPadRight = 0x92,    /* On/Off Control */
		DPadLeft = 0x93,    /* On/Off Control */
		/* 0x94 - 0xFFFF Reserved */
		Reserved = 0xFFFF
	};
}


#endif /* __RAYNE_HID_H_ */
