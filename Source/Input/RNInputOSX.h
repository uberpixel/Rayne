//
//  RNInputOSX.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INPUTOSX_H_
#define __RAYNE_INPUTOSX_H_

#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/IOCFPlugIn.h>
#include "../Base/RNBase.h"
#include "RNInputManager.h"

namespace RN
{
	class OSXPlatformDevice : public InputDevice
	{
	public:
		OSXPlatformDevice(const Descriptor &descriptor, io_object_t object, CFDictionaryRef properties);
		~OSXPlatformDevice();

		void Update() override;
		bool __Activate() override;
		bool __Deactivate() override;

	private:
		void BuildControlTree(InputControl *parent, CFDictionaryRef properties);

		io_object_t _object;

		IOCFPlugInInterface **_pluginInterface;
		IOHIDDeviceInterface **_deviceInterface;
		IOHIDQueueInterface **_deviceQueue;

		RNDeclareMeta(OSXPlatformDevice);
	};

	class OSXPlatformControl
	{
	public:
		friend class OSXPlatformDevice;

		OSXPlatformControl(IOHIDElementCookie cookie) :
			_cookie(cookie)
		{}

	private:
		IOHIDElementCookie _cookie;
	};

	class OSXPlatformButtonControl : public ButtonControl, public OSXPlatformControl
	{
	public:
		OSXPlatformButtonControl(const String *name, IOHIDElementCookie cookie);

		void Start() { ButtonControl::Start(); }
		void End() { ButtonControl::End(); }

	private:
		RNDeclareMeta(OSXPlatformButtonControl)
	};
}


#endif /* __RAYNE_INPUTOSX_H_ */
