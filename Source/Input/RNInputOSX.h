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
	struct HIDElement
	{
		HIDElement(IOHIDElementRef element, InputControl *control);
		~HIDElement();

		HIDElement(HIDElement &&other) = default;
		HIDElement &operator =(HIDElement &&other) = default;

		IOHIDElementRef element;
		IOHIDElementCookie cookie;

		CFIndex logicalMinimum;
		CFIndex logicalMaximum;

		CFIndex physicalMinimum;
		CFIndex physicalMaximum;

		uint32 usage;
		uint32 usagePage;

		size_t reportSize;
		size_t reportCount;

		InputControl *control;

		void HandleValue(IOHIDValueRef value);
	};


	class OSXPlatformDevice : public InputDevice
	{
	public:
		OSXPlatformDevice(const Descriptor &descriptor, IOHIDDeviceRef device);

		~OSXPlatformDevice();

		void Update() override;
		bool __Activate() override;
		bool __Deactivate() override;

		IOHIDDeviceRef GetRawDevice() const { return _device; }

	private:
		void BuildControlTree(InputControl *parent, CFArrayRef elements);

		std::vector<HIDElement *> _allElements;
		std::unordered_map<IOHIDElementCookie, HIDElement *> _elements;

		IOHIDQueueRef _queue;
		IOHIDDeviceRef _device;

		size_t _buttonCount;
		size_t _sliderCount;

		size_t _deltaAxisCount;
		size_t _linearAxisCount;
		size_t _rotationAxisCount;

		RNDeclareMeta(OSXPlatformDevice);
	};
}


#endif /* __RAYNE_INPUTOSX_H_ */
