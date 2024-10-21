//
//  RNInputOSX.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Base/RNBaseInternal.h"
#include "../Debug/RNLogger.h"
#include "RNInputOSX.h"

#define kInputQueueSize 512

namespace RN
{
	RNDefineMeta(OSXPlatformDevice, InputDevice)

	static HIDUsagePage __IOHIDDeviceGetUsagePage(IOHIDDeviceRef device)
	{
		CFNumberRef value = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDDeviceUsagePageKey));
		if(!value)
			value = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsagePageKey));

		HIDUsagePage usagePage;
		CFNumberGetValue(value, kCFNumberSInt16Type, &usagePage);

		return usagePage;
	}
	static uint16 __IOHIDDeviceGetUsage(IOHIDDeviceRef device)
	{
		CFNumberRef value = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDDeviceUsageKey));
		if(!value)
			value = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDPrimaryUsageKey));

		uint16 usage;
		CFNumberGetValue(value, kCFNumberSInt16Type, &usage);

		return usage;
	}

	OSXHIDDevice::OSXHIDDevice(IOHIDDeviceRef device) :
		HIDDevice(__IOHIDDeviceGetUsagePage(device), __IOHIDDeviceGetUsage(device)),
		_device(device),
		_openCount(0)
	{
		CFRetain(device);
		Open();
	}

	OSXHIDDevice::~OSXHIDDevice()
	{
		for(auto iterator : _inputReports)
			iterator.second->Release();

		CFRelease(_device);
	}

	void OSXHIDDevice::Open()
	{
		if((_openCount ++) == 0)
		{
			IOReturn result = IOHIDDeviceOpen(_device, kIOHIDOptionsTypeNone);
			if(result != kIOReturnSuccess)
			{
				_openCount = 0;
				throw HIDOpenException("Failed to open HID device");
			}

			Retain();

			_featureReportLength = GetSizeWithKey(CFSTR(kIOHIDMaxFeatureReportSizeKey));
			_inputReportLength = GetSizeWithKey(CFSTR(kIOHIDMaxInputReportSizeKey));
			_outputReportLength = GetSizeWithKey(CFSTR(kIOHIDMaxOutputReportSizeKey));

			if(_featureReportLength)
				_featureReportBuffer = new uint8[_featureReportLength];
			if(_inputReportLength)
				_inputReportBuffer = new uint8[_inputReportLength];
			if(_outputReportLength)
				_outputReportBuffer = new uint8[_outputReportLength];

			IOHIDDeviceRegisterInputReportCallback(_device, _inputReportBuffer, _inputReportLength, &OSXHIDDevice::InputReportCallback, this);
			IOHIDDeviceScheduleWithRunLoop(_device, CFRunLoopGetMain(), kCFRunLoopCommonModes);
		}
	}
	void OSXHIDDevice::Close()
	{
		if((-- _openCount) == 0)
		{
			IOHIDDeviceRegisterInputReportCallback(_device, nullptr, 0, nullptr, this); //While it causes a warning, the null here are all correct!
			IOHIDDeviceUnscheduleFromRunLoop(_device, CFRunLoopGetMain(), kCFRunLoopCommonModes);
			IOHIDDeviceClose(_device, kIOHIDOptionsTypeNone);

			delete[] _featureReportBuffer;
			delete[] _inputReportBuffer;
			delete[] _outputReportBuffer;

			_featureReportBuffer = nullptr;
			_inputReportBuffer = nullptr;
			_outputReportBuffer = nullptr;

			Release();
		}
	}

	void OSXHIDDevice::InputReportCallback(void *context, __unused IOReturn result, __unused void *deviceRef, __unused IOHIDReportType type, uint32_t reportID, uint8_t *report, CFIndex length)
	{
		OSXHIDDevice *device = static_cast<OSXHIDDevice *>(context);
		device->HandleInputReport(reportID, report, length);
	}
	void OSXHIDDevice::HandleInputReport(uint32_t reportID, uint8_t *report, CFIndex length)
	{
		auto iterator = _inputReports.find(reportID);
		if(iterator != _inputReports.end())
		{
			iterator->second->Release();
			_inputReports.erase(iterator);
		}

		Data *data = new Data(report, static_cast<size_t>(length));
		_inputReports[reportID] = data;
	}


	Data *OSXHIDDevice::ReadReport(uint32 reportID) const
	{
		auto iterator = _inputReports.find(reportID);
		if(iterator != _inputReports.end())
		{
			Data *data = iterator->second;
			_inputReports.erase(iterator);

			return data->Autorelease();
		}

		return nullptr;
	}

	size_t OSXHIDDevice::WriteReport(uint32 reportID, const Data *data)
	{
		IOReturn result = IOHIDDeviceSetReport(_device, kIOHIDReportTypeOutput, reportID, (const uint8_t *)data->GetBytes(), data->GetLength());
		if(result != kIOReturnSuccess)
			throw HIDWriteException("Failed to write report");

		return _outputReportLength;
	}

	Data *OSXHIDDevice::ReadFeatureReport(uint32 reportID) const
	{
		Data *data = new Data(nullptr, _featureReportLength);
		data->Autorelease();

		CFIndex length = static_cast<CFIndex>(_featureReportLength);

		IOReturn result = IOHIDDeviceGetReport(_device, kIOHIDReportTypeFeature, reportID, (uint8_t *)data->GetBytes(), &length);
		if(result != kIOReturnSuccess)
			throw HIDWriteException("Failed to write report");

		return data;
	}

	uint32 OSXHIDDevice::GetSizeWithKey(CFStringRef key) const
	{
		CFNumberRef number = (CFNumberRef)IOHIDDeviceGetProperty(_device, key);
		if(number && CFGetTypeID(number) == CFNumberGetTypeID())
		{
			uint32 value;
			CFNumberGetValue(number, kCFNumberSInt32Type, &value);

			return value;
		}

		return 0;
	}

	const String *OSXHIDDevice::GetStringWithKey(CFStringRef key) const
	{
		CFStringRef string = (CFStringRef)IOHIDDeviceGetProperty(_device, key);
		if(string && CFGetTypeID(string) == CFStringGetTypeID())
		{
			const size_t size = static_cast<size_t>(CFStringGetLength(string) + 1);
			char *data = new char[size];

			CFStringGetCString(string, data, size, kCFStringEncodingASCII);

			String *result = new String(data);

			delete[] data;

			return result->Autorelease();
		}

		return nullptr;
	}

	const String *OSXHIDDevice::GetManufacturerString() const
	{
		return GetStringWithKey(CFSTR(kIOHIDManufacturerKey));
	}
	const String *OSXHIDDevice::GetProductString() const
	{
		return GetStringWithKey(CFSTR(kIOHIDProductKey));
	}
	const String *OSXHIDDevice::GetSerialString() const
	{
		return GetStringWithKey(CFSTR(kIOHIDSerialNumberKey));
	}

	uint32 OSXHIDDevice::GetVendorID() const
	{
		return GetSizeWithKey(CFSTR(kIOHIDVendorIDKey));
	}
	uint32 OSXHIDDevice::GetProductID() const
	{
		return GetSizeWithKey(CFSTR(kIOHIDProductIDKey));
	}



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



	OSXPlatformDevice::OSXPlatformDevice(const Descriptor &descriptor, IOHIDDeviceRef device) :
		InputDevice(descriptor),
		_queue(IOHIDQueueCreate(kCFAllocatorDefault, device, 32, kIOHIDOptionsTypeNone)),
		_device(device),
		_hasDataAvailable(false),
		_buttonCount(0),
		_sliderCount(0),
		_deltaAxisCount(0),
		_linearAxisCount(0),
		_rotationAxisCount(0)
	{
		if(!_device)
			return;
			
		CFRetain(_device);

		CFArrayRef elements = IOHIDDeviceCopyMatchingElements(_device, NULL, kIOHIDOptionsTypeNone);
        if(elements)
        {
            BuildControlTree(this, elements);
            CFRelease(elements);
        }

		// Prepare the queue
		for(HIDElement *element : _allElements)
			IOHIDQueueAddElement(_queue, element->element);
	}

	OSXPlatformDevice::~OSXPlatformDevice()
	{
		if(!_device)
			return;
		
		CFRelease(_device);
		CFRelease(_queue);

		IOHIDDeviceClose(_device, kIOHIDOptionsTypeNone);

		for(HIDElement *element : _allElements)
			delete element;
	}

	void OSXPlatformDevice::BuildControlTree(InputControl *parent, CFArrayRef elements)
	{
		size_t count = static_cast<size_t>(CFArrayGetCount(elements));

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

								default:
									break;
							}

							break;
						}
						case kHIDPage_Button:
						{
							String *name = RNSTR("Button " << (++ _buttonCount));
							control = new ButtonControl(name, InputControl::Type::Button);
						}

						default:
							break;
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

		while(_hasDataAvailable)
		{
			IOHIDValueRef value = IOHIDQueueCopyNextValue(_queue);
			if(!value)
			{
				_hasDataAvailable = false;
				break;
			}

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

	void OSXPlatformDevice::DataAvailableCallback(void *context, __unused IOReturn result, __unused void *sender)
	{
		OSXPlatformDevice *device = static_cast<OSXPlatformDevice *>(context);
		device->_hasDataAvailable = true;
	}

	bool OSXPlatformDevice::__Activate()
	{
		if(IOHIDDeviceOpen(_device, kIOHIDOptionsTypeNone) != kIOReturnSuccess)
			return false;

		IOHIDQueueStart(_queue);
		IOHIDQueueRegisterValueAvailableCallback(_queue, &OSXPlatformDevice::DataAvailableCallback, this);
		IOHIDQueueScheduleWithRunLoop(_queue, CFRunLoopGetMain(), kCFRunLoopDefaultMode);

		return true;
	}
	bool OSXPlatformDevice::__Deactivate()
	{
		IOHIDQueueStop(_queue);
		IOHIDQueueUnscheduleFromRunLoop(_queue, CFRunLoopGetMain(), kCFRunLoopDefaultMode);

		IOHIDDeviceClose(_device, kIOHIDOptionsTypeNone);

		return true;
	}

	// OSXMouseDevice

	OSXMouseDevice::OSXMouseDevice(const Descriptor &descriptor, IOHIDDeviceRef device) :
		OSXPlatformDevice(descriptor, device)
	{
		CGEventMask mask = CGEventMaskBit(kCGEventLeftMouseDown) |
			CGEventMaskBit(kCGEventLeftMouseUp) |
			CGEventMaskBit(kCGEventRightMouseDown) |
			CGEventMaskBit(kCGEventRightMouseUp) |
			CGEventMaskBit(kCGEventMouseMoved) |
			CGEventMaskBit(kCGEventLeftMouseDragged) |
			CGEventMaskBit(kCGEventRightMouseDragged) |
			CGEventMaskBit(kCGEventScrollWheel) |
			CGEventMaskBit(kCGEventOtherMouseDown) |
			CGEventMaskBit(kCGEventOtherMouseUp) |
			CGEventMaskBit(kCGEventOtherMouseDragged);

		_eventTap = CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly, mask, &OSXMouseDevice::EventCallback, this);
		_runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, _eventTap, 0);
		
		if(device == nullptr)
		{
			DeltaAxisControl *xAxisControl = new DeltaAxisControl(RNSTR(kDeltaAxisName[0] << "1"), AxisControl::Axis::X);
			AddControl(xAxisControl);
			xAxisControl->Release();
			
			DeltaAxisControl *yAxisControl = new DeltaAxisControl(RNSTR(kDeltaAxisName[1] << "2"), AxisControl::Axis::Y);
			AddControl(yAxisControl);
			yAxisControl->Release();
			
			ButtonControl *leftButtonControl = new ButtonControl(RNSTR("Button " << 1), ButtonControl::Type::Button);
			AddControl(leftButtonControl);
			leftButtonControl->Release();
			
			ButtonControl *rightButtonControl = new ButtonControl(RNSTR("Button " << 2), ButtonControl::Type::Button);
			AddControl(rightButtonControl);
			rightButtonControl->Release();
			
			ButtonControl *middleButtonControl = new ButtonControl(RNSTR("Button " << 3), ButtonControl::Type::Button);
			AddControl(middleButtonControl);
			middleButtonControl->Release();
		}

		_deltaXAxis = GetControlWithName<DeltaAxisControl>(RNSTR(kDeltaAxisName[0] << "1"));
		_deltaYAxis = GetControlWithName<DeltaAxisControl>(RNSTR(kDeltaAxisName[1] << "2"));

		_buttonControls = new Array();

		for(size_t i = 1;; i ++)
		{
			ButtonControl *control = GetControlWithName<ButtonControl>(RNSTR("Button " << i));
			if(!control)
				break;

			_buttonControls->AddObject(control);
		}
	}
	OSXMouseDevice::~OSXMouseDevice()
	{
		_buttonControls->Release();

		CFRelease(_eventTap);
		CFRelease(_runLoopSource);
	}

	bool OSXMouseDevice::__Activate()
	{
		CFRunLoopAddSource(CFRunLoopGetCurrent(), _runLoopSource, kCFRunLoopCommonModes);
		CGEventTapEnable(_eventTap, true);

		Reset();

		return true;
	}
	bool OSXMouseDevice::__Deactivate()
	{
		CGEventTapEnable(_eventTap, false);
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), _runLoopSource, kCFRunLoopCommonModes);

		Reset();

		return true;
	}

	void OSXMouseDevice::Update()
	{
		InputDevice::Update();

		if(_deltaXAxis)
			_deltaXAxis->SetValue(_lastDelta.x);
		if(_deltaYAxis)
			_deltaYAxis->SetValue(_lastDelta.y);

		for(auto &pair : _buttonEvents)
		{
            if(pair.first >= _buttonControls->GetCount()) continue;
            
			ButtonControl *control = _buttonControls->GetObjectAtIndex<ButtonControl>(pair.first);
			control->SetPressed(pair.second);
		}

		Reset();
	}

	CGEventRef OSXMouseDevice::EventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *context)
	{
		OSXMouseDevice *device = reinterpret_cast<OSXMouseDevice *>(context);
		device->HandleEvent(proxy, type, event, context);

		return event;
	}

	void OSXMouseDevice::HandleEvent(CGEventTapProxy proxy, CGEventType type, CGEventRef cgEvent, void *context)
	{
		if(type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput)
			return;

		@autoreleasepool
		{
			NSEvent *event = [NSEvent eventWithCGEvent:cgEvent];

			switch(type)
			{
				case kCGEventLeftMouseDown:
				case kCGEventRightMouseDown:
				case kCGEventOtherMouseDown:
					_buttonEvents.emplace_back(std::make_pair(static_cast<uint32>([event buttonNumber]), true));
					break;

				case kCGEventLeftMouseUp:
				case kCGEventRightMouseUp:
				case kCGEventOtherMouseUp:
					_buttonEvents.emplace_back(std::make_pair(static_cast<uint32>([event buttonNumber]), false));
					break;

				case kCGEventMouseMoved:
					_lastDelta.x += [event deltaX] * 2;
					_lastDelta.y += [event deltaY] * 2;
					break;

				case kCGEventScrollWheel:
					_lastMouseWheel += Vector2([event deltaX], [event deltaY]);
					break;

				case kCGEventLeftMouseDragged:
				case kCGEventRightMouseDragged:
				case kCGEventOtherMouseDragged:

					break;

				default:
					break; // Shouldn't execute
			}
		}
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
	static bool _hasMouse = false;

	void OSXPlatformHIDDeviceAdded(__unused void *context, __unused IOReturn result, __unused void *sender, IOHIDDeviceRef deviceRef)
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

				if(!productValue || !vendorValue)
					break;

				uint32 product = 0;
				uint32 vendor = 0;

				String *productName;
				String *vendorName;

				{
					if(productID) CFNumberGetValue(productID, kCFNumberSInt32Type, &product);
					if(vendorID) CFNumberGetValue(vendorID, kCFNumberSInt32Type, &vendor);
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
					{
						if(_hasMouse)
							break;

						InputDevice::Descriptor descriptor(InputDevice::Category::Mouse);
						descriptor.SetVendor(vendorName);
						descriptor.SetName(productName);
						descriptor.SetVendorID(Number::WithUint32(vendor));
						descriptor.SetProductID(Number::WithUint32(product));

						OSXMouseDevice *device = new OSXMouseDevice(descriptor, deviceRef);
						device->Register();
						device->Activate();

						_foundDevices->AddObject(device->Autorelease());
						_hasMouse = true;
						break;
					}

					default:
					{
						OSXHIDDevice *device = new OSXHIDDevice(deviceRef);
						device->Register();
						device->Release();

						break;
					}
				}

				if(valid)
				{
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

			default:
				break;
		}
	}
	void OSXPlatformHIDDeviceRemoved(__unused void *context, __unused IOReturn result, __unused void *sender, IOHIDDeviceRef deviceRef)
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
		
		
/*		if(!_hasMouse)
		{
			InputDevice::Descriptor descriptor(InputDevice::Category::Mouse);
			descriptor.SetVendor(RNCSTR("blubb"));
			descriptor.SetName(RNCSTR("mouse"));
			descriptor.SetVendorID(Number::WithUint32(1));
			descriptor.SetProductID(Number::WithUint32(1));
			
			OSXMouseDevice *device = new OSXMouseDevice(descriptor, nullptr);
			device->Register();
			device->Activate();
		}*/
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
