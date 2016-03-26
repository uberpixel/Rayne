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
}


#endif /* __RAYNE_HID_H_ */
