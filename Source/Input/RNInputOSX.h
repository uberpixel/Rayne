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
#include "RNHIDDevice.h"
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

		static void DataAvailableCallback(void *context, IOReturn result, void *sender);

		std::vector<HIDElement *> _allElements;
		std::unordered_map<IOHIDElementCookie, HIDElement *> _elements;

		IOHIDQueueRef _queue;
		IOHIDDeviceRef _device;

		bool _hasDataAvailable;

		size_t _buttonCount;
		size_t _sliderCount;

		size_t _deltaAxisCount;
		size_t _linearAxisCount;
		size_t _rotationAxisCount;

		RNDeclareMeta(OSXPlatformDevice);
	};

	class OSXMouseDevice : public OSXPlatformDevice
	{
	public:
		RNAPI OSXMouseDevice(const Descriptor &descriptor, IOHIDDeviceRef device);
		RNAPI ~OSXMouseDevice();

		RNAPI void Update() override;

		RNAPI bool __Activate() override;
		RNAPI bool __Deactivate() override;

	private:
		static CGEventRef EventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *context);

		void HandleEvent(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *context);
		void Reset()
		{
			_lastDelta = Vector2(0);
			_lastMouseWheel = Vector2(0);
			_buttonEvents.clear();
		}

		CFMachPortRef _eventTap;
		CFRunLoopSourceRef _runLoopSource;

		Array *_buttonControls;
		DeltaAxisControl *_deltaXAxis;
		DeltaAxisControl *_deltaYAxis;

		Vector2 _lastDelta;
		Vector2 _lastMouseWheel;

		std::vector<std::pair<size_t, bool>> _buttonEvents;
	};




	class OSXHIDDevice : public HIDDevice
	{
	public:
		RNAPI OSXHIDDevice(IOHIDDeviceRef device);
		RNAPI ~OSXHIDDevice();

		RNAPI void Open() final;
		RNAPI void Close() final;

		RNAPI virtual Data *ReadReport(uint32 reportID) const final;
		RNAPI virtual Data *ReadFeatureReport(uint32 reportID) const final;

		RNAPI virtual size_t WriteReport(uint32 reportID, const Data *data) final;

		RNAPI const String *GetManufacturerString() const final;
		RNAPI const String *GetProductString() const final;
		RNAPI const String *GetSerialString() const final;

		size_t GetInputReportLength() const { return _inputReportLength; }
		size_t GetOutputReportLength() const { return _outputReportLength; }
		size_t GetFeatureReportLength() const { return _featureReportLength; }

		RNAPI uint32 GetVendorID() const final;
		RNAPI uint32 GetProductID() const final;

	private:
		void HandleInputReport(uint32_t reportID, uint8_t *report, CFIndex length);
		static void InputReportCallback(void *context, IOReturn result, void *deviceRef, IOHIDReportType type, uint32_t reportID, uint8_t *report, CFIndex length);

		const String *GetStringWithKey(CFStringRef key) const;
		uint32 GetSizeWithKey(CFStringRef key) const;

		IOHIDDeviceRef _device;
		uint32 _openCount;

		uint32 _featureReportLength;
		uint8 *_featureReportBuffer;

		uint32 _inputReportLength;
		uint8 *_inputReportBuffer;

		uint32 _outputReportLength;
		uint8 *_outputReportBuffer;

		mutable std::map<uint32, Data *> _inputReports;
	};


}


#endif /* __RAYNE_INPUTOSX_H_ */
