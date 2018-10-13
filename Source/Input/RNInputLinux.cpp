//
//  RNInputLinux.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Debug/RNLogger.h"
#include "RNInputLinux.h"
/*#include <libudev.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <fcntl.h>
#include <unistd.h>*/

#define kInputQueueSize 512

namespace RN
{
	static struct udev *_udev = nullptr;
	static struct udev_monitor *_udevMonitor = nullptr;
	static Array *_foundDevices = nullptr;

	RNDefineMeta(LinuxPlatformDevice, InputDevice)

    LinuxHIDDevice::~LinuxHIDDevice()
    {

    }

    void LinuxHIDDevice::Open()
    {

    }
    void LinuxHIDDevice::Close()
    {

    }


    Data *LinuxHIDDevice::ReadReport(uint32 reportID) const
    {
        return nullptr;
    }

    size_t LinuxHIDDevice::WriteReport(uint32 reportID, const Data *data)
    {
        return 0;
    }

    Data *LinuxHIDDevice::ReadFeatureReport(uint32 reportID) const
    {
        return nullptr;
    }

    const String *LinuxHIDDevice::GetManufacturerString() const
    {
        return nullptr;
    }
    const String *LinuxHIDDevice::GetProductString() const
    {
        return nullptr;
    }
    const String *LinuxHIDDevice::GetSerialString() const
    {
        return nullptr;
    }

    uint32 LinuxHIDDevice::GetVendorID() const
    {
        return 0;
    }
    uint32 LinuxHIDDevice::GetProductID() const
    {
        return 0;
    }


    const char *kKeyboardButtonName[] =
            {
					"0x00", "Escape", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Erase", "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
					"KEY_LEFTBRACE", "KEY_RIGHTBRACE", "Enter", "Left Control", "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "KEY_GRAVE", "Left Shift", "\\",
					"Z", "X", "C", "V", "B", "N", "M", ",", ".", "/", "Right Shift", "KEY_KPASTERISK", "Left Option", "Space", "Caps-Lockable", "F1", "F2", "F3", "F4", "F5", "F6",
					"F7", "F8", "F9", "F10", "Num-Lockable", "Scroll-Lockable", "Pad 7", "Pad 8", "Pad 9", "Pad -", "Pad 4", "Pad 5", "Pad 6", "Pad +", "Pad 1", "Pad 2", "Pad 3", "Pad 0",
					"Pad .", "EMPTY SLOT", "KEY_ZENKAKUHANKAKU", "KEY_102ND", "F11", "F12", "KEY_RO", "KEY_KATAKANA", "KEY_HIRAGANA", "KEY_HENKAN", "KEY_KATAKANAHIRAGANA", "KEY_MUHENKAN",
					"KEY_KPJPCOMMA", "Enter", "Right Control", "Pad /", "KEY_SYSRQ", "Right Option", "KEY_LINEFEED", "Home", "Up", "Page Up", "Left", "Right", "End",
					"Down", "Page Down", "Insert", "Del", "KEY_MACRO", "KEY_MUTE", "KEY_VOLUMEDOWN", "KEY_VOLUMEUP", "KEY_POWER", "Pad =", "KEY_KPPLUSMINUS", "Pause", "KEY_SCALE",
					"KEY_KPCOMMA", "KEY_HANGEUL", "KEY_HANJA", "KEY_YEN", "KEY_LEFTMETA", "KEY_RIGHTMETA", "KEY_COMPOSE", "KEY_STOP", "KEY_AGAIN", "KEY_PROPS", "KEY_UNDO", "KEY_FRONT", "KEY_COPY",
					"KEY_OPEN", "KEY_PASTE", "KEY_FIND", "KEY_CUT", "KEY_HELP", "KEY_MENU", "KEY_CALC", "KEY_SETUP", "KEY_SLEEP", "KEY_WAKEUP", "KEY_FILE", "KEY_SENDFILE", "KEY_DELETEFILE",
					"KEY_XFER", "KEY_PROG1", "KEY_PROG2", "KEY_WWW", "KEY_MSDOS", "KEY_COFFEE", "KEY_ROTATE_DISPLAY", "KEY_CYCLEWINDOWS", "KEY_MAIL", "KEY_BOOKMARKS", "KEY_COMPUTER",
					"KEY_BACK", "KEY_FORWARD", "KEY_CLOSECD", "KEY_EJECTCD", "KEY_EJECTCLOSECD", "KEY_NEXTSONG", "KEY_PLAYPAUSE", "KEY_PREVIOUSSONG", "KEY_STOPCD", "KEY_RECORD", "KEY_REWIND",
					"KEY_PHONE", "KEY_ISO", "KEY_CONFIG", "KEY_HOMEPAGE", "KEY_REFRESH", "KEY_EXIT", "KEY_MOVE", "KEY_EDIT", "KEY_SCROLLUP", "KEY_SCROLLDOWN", "KEY_KPLEFTPAREN", "KEY_KPRIGHTPAREN",
					"KEY_NEW", "KEY_REDO", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "EMPTY SLOT", "EMPTY SLOT", "EMPTY SLOT", "EMPTY SLOT", "EMPTY SLOT",
					"KEY_PLAYCD", "KEY_PAUSECD", "KEY_PROG3", "KEY_PROG4", "KEY_DASHBOARD", "KEY_SUSPEND", "KEY_CLOSE", "KEY_PLAY", "KEY_FASTFORWARD", "KEY_BASSBOOST", "KEY_PRINT", "KEY_HP",
					"KEY_CAMERA", "KEY_SOUND", "KEY_QUESTION", "KEY_EMAIL", "KEY_CHAT", "KEY_SEARCH", "KEY_CONNECT", "KEY_FINANCE", "KEY_SPORT", "KEY_SHOP", "KEY_ALTERASE", "KEY_CANCEL",
					"KEY_BRIGHTNESSDOWN", "KEY_BRIGHTNESSUP", "KEY_MEDIA", "KEY_SWITCHVIDEOMODE", "KEY_KBDILLUMTOGGLE", "KEY_KBDILLUMDOWN", "KEY_KBDILLUMUP", "KEY_SEND", "KEY_REPLY", "KEY_FORWARDMAIL",
					"KEY_SAVE", "KEY_DOCUMENTS", "KEY_BATTERY", "KEY_BLUETOOTH", "KEY_WLAN", "KEY_UWB", "KEY_UNKNOWN", "KEY_VIDEO_NEXT", "KEY_VIDEO_PREV", "KEY_BRIGHTNESS_CYCLE", "KEY_BRIGHTNESS_AUTO",
					"KEY_DISPLAY_OFF", "KEY_WWAN", "KEY_RFKILL", "KEY_MICMUTE", "EMPTY SLOT", "EMPTY SLOT", "EMPTY SLOT", "EMPTY SLOT", "EMPTY SLOT", "EMPTY SLOT", "EMPTY SLOT"
            };

	const char *kMouseButtonName[] =
			{
					"Mouse Left", "Mouse Right", "Mouse Middle", "Mouse Side", "Mouse Extra", "Mouse Forward", "Mouse Back", "Mouse Task"
			};

    const char *kRelativeAxisName[] =
            {
                    "X-Delta ", "Y-Delta ", "Z-Delta ", "RX-Delta ", "RY-Delta ", "RZ-Delta ", "HWheel", "Dial", "Wheel", "Misc"
            };

	const char *kAbsoluteAxisName[] =
			{
					"X-Axis ", "Y-Axis ", "Z-Axis ", "RX-Axis ", "RY-Axis ", "RZ-Axis "
			};



    LinuxPlatformDevice::LinuxPlatformDevice(const Descriptor &descriptor) ://, udev_device *device) :
            InputDevice(descriptor),
			//_udevDevice(device),
            _hasDataAvailable(false),
            _buttonCount(0),
            _sliderCount(0),
            _deltaAxisCount(0),
            _linearAxisCount(0),
            _rotationAxisCount(0)
    {
 /*       if(!_udevDevice)
            return;

        udev_device_ref(_udevDevice);

		struct udev_enumerate *enumerate = udev_enumerate_new(_udev);
		udev_enumerate_add_match_parent(enumerate, _udevDevice);
		//udev_enumerate_add_match_subsystem(enumerate, "input");
		udev_enumerate_scan_devices(enumerate);
		struct udev_list_entry *device_entry_list;
		device_entry_list = udev_enumerate_get_list_entry(enumerate);
		struct udev_list_entry *device_entry;
		for(device_entry = device_entry_list; device_entry; device_entry = udev_list_entry_get_next(device_entry))
		{
			const char *path;
			path = udev_list_entry_get_name(device_entry);
			struct udev_device *child;
			child = udev_device_new_from_syspath(_udev, path);

			path = udev_device_get_devnode(child);
			if(!path) continue;

			_fileHandle = open(path, O_RDONLY|O_NONBLOCK);
			int result = 1;
			result = libevdev_new_from_fd(_fileHandle, &_device);
			if(result < 0)
			{
				RNDebug("Shitfuck: " << path);
				if(_fileHandle != -1) close(_fileHandle);
				_fileHandle = -1;
				continue;
			}

			BuildControlTree();
			RNDebug("Working!? " << path);
			break;
		}

		udev_enumerate_unref(enumerate);*/
    }

    LinuxPlatformDevice::~LinuxPlatformDevice()
    {
 //       if(!_udevDevice)
 //           return;

//		libevdev_free(_device);
		//close(_fileHandle);

//        udev_device_unref(_udevDevice);

 /*
        for(HIDElement *element : _allElements)
            delete element;*/
    }

    void LinuxPlatformDevice::AddControl(InputControl *control)
	{
		if(control)
		{
/*			HIDElement *hidElement = new HIDElement(element, control);

			_allElements.push_back(hidElement);
			_elements.emplace(cookie, hidElement);*/

			InputDevice::AddControl(control);
			control->Release();

/*			switch(control->GetType())
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
			}*/
		}
	}

    void LinuxPlatformDevice::BuildControlTree()
    {
 /*   	for(int eventType = 0; eventType < EV_CNT; eventType++)
		{
    		if(libevdev_has_event_type(_device, eventType))
			{
    			switch(eventType)
				{
					case EV_SYN:
						break;

					case EV_KEY:
						for(int code = 0; code < 255; code++) //There are many more such as mouse buttons, joystick buttons, ... all in the higher numbers
						{
							InputControl *control = new ButtonControl(RNSTR(kKeyboardButtonName[code]), InputControl::Type::KeyButton);
							AddControl(control);
						}
						for(int code = BTN_MOUSE; code <= BTN_TASK; code++)
						{
							InputControl *control = new ButtonControl(RNSTR(kMouseButtonName[code-BTN_MOUSE]), InputControl::Type::KeyButton);
							AddControl(control);
						}
						break;

					case EV_REL:
						for(int code = 0; code < REL_MAX; code++)
						{
							InputControl *control = nullptr;
							switch(code)
							{
								case REL_X:
									control = new DeltaAxisControl(RNSTR(kRelativeAxisName[code] << (++ _deltaAxisCount)), AxisControl::Axis::X);
									break;
								case REL_Y:
									control = new DeltaAxisControl(RNSTR(kRelativeAxisName[code] << (++ _deltaAxisCount)), AxisControl::Axis::Y);
									break;
								case REL_Z:
									control = new DeltaAxisControl(RNSTR(kRelativeAxisName[code] << (++ _deltaAxisCount)), AxisControl::Axis::Z);
									break;
								case REL_RX:
									control = new RotationAxisControl(RNSTR(kRelativeAxisName[code] << (++ _rotationAxisCount)), AxisControl::Axis::X);
									break;
								case REL_RY:
									control = new RotationAxisControl(RNSTR(kRelativeAxisName[code] << (++ _rotationAxisCount)), AxisControl::Axis::Y);
									break;
								case REL_RZ:
									control = new RotationAxisControl(RNSTR(kRelativeAxisName[code] << (++ _rotationAxisCount)), AxisControl::Axis::Z);
									break;
								case REL_WHEEL:
									control = new DeltaAxisControl(RNSTR(kRelativeAxisName[code] << (++ _deltaAxisCount)), AxisControl::Axis::Z);
									break;

								default:
									continue;
							}

							AddControl(control);
						}
						break;

					case EV_ABS:
						for(int code = 0; code <= ABS_RZ; code++) //There are many more absolute axes, only going for most basic ones for now
						{
							InputControl *control = nullptr;
							switch(code)
							{
								case ABS_X:
									control = new LinearAxisControl(RNSTR(kAbsoluteAxisName[code] << (++ _linearAxisCount)), AxisControl::Axis::X);
									break;
								case ABS_Y:
									control = new LinearAxisControl(RNSTR(kAbsoluteAxisName[code] << (++ _linearAxisCount)), AxisControl::Axis::Y);
									break;
								case ABS_Z:
									control = new LinearAxisControl(RNSTR(kAbsoluteAxisName[code] << (++ _linearAxisCount)), AxisControl::Axis::Z);
									break;
								case ABS_RX:
									control = new RotationAxisControl(RNSTR(kAbsoluteAxisName[code] << (++ _rotationAxisCount)), AxisControl::Axis::X);
									break;
								case ABS_RY:
									control = new RotationAxisControl(RNSTR(kAbsoluteAxisName[code] << (++ _rotationAxisCount)), AxisControl::Axis::Y);
									break;
								case ABS_RZ:
									control = new RotationAxisControl(RNSTR(kAbsoluteAxisName[code] << (++ _rotationAxisCount)), AxisControl::Axis::Z);
									break;

								default:
									continue;
							}

							AddControl(control);
						}
						break;

					default:
						break;
				}
			}
		}*/

/*        size_t count = static_cast<size_t>(CFArrayGetCount(elements));

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
        }*/
    }

    void LinuxPlatformDevice::Update()
    {
        InputDevice::Update();

/*        while(_hasDataAvailable)
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
        }*/
    }

/*    void LinuxPlatformDevice::DataAvailableCallback(void *context, __unused IOReturn result, __unused void *sender)
    {
        OSXPlatformDevice *device = static_cast<OSXPlatformDevice *>(context);
        device->_hasDataAvailable = true;
    }*/

    bool LinuxPlatformDevice::__Activate()
    {
/*        if(IOHIDDeviceOpen(_device, kIOHIDOptionsTypeNone) != kIOReturnSuccess)
            return false;

        IOHIDQueueStart(_queue);
        IOHIDQueueRegisterValueAvailableCallback(_queue, &OSXPlatformDevice::DataAvailableCallback, this);
        IOHIDQueueScheduleWithRunLoop(_queue, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
*/
        return true;
    }
    bool LinuxPlatformDevice::__Deactivate()
    {
 /*       IOHIDQueueStop(_queue);
        IOHIDQueueUnscheduleFromRunLoop(_queue, CFRunLoopGetMain(), kCFRunLoopDefaultMode);

        IOHIDDeviceClose(_device, kIOHIDOptionsTypeNone);*/

        return true;
    }


    // Device management
/*    InputDevice::Category LinuxPlatformGetHIDDeviceCategory(udev_device *device)
    {
        InputDevice::Category category = 0;

        struct udev_enumerate *enumerate = udev_enumerate_new(_udev);
        udev_enumerate_add_match_parent(enumerate, device);
        udev_enumerate_add_match_subsystem(enumerate, "input");
        udev_enumerate_scan_devices(enumerate);
        struct udev_list_entry *device_entry_list;
        device_entry_list = udev_enumerate_get_list_entry(enumerate);
        struct udev_list_entry *device_entry;
        for(device_entry = device_entry_list; device_entry; device_entry = udev_list_entry_get_next(device_entry))
        {
            const char *path;
            path = udev_list_entry_get_name(device_entry);
            struct udev_device *child;
			child = udev_device_new_from_syspath(_udev, path);

			String *classValue = RNSTR(udev_device_get_property_value(child, "ID_INPUT_JOYSTICK"));
			if(classValue && classValue->IsEqual(RNCSTR("1")))
			{
				category |= InputDevice::Category::Joystick;
			}

			classValue = RNSTR(udev_device_get_property_value(child, "ID_INPUT_MOUSE"));
			if(classValue && classValue->IsEqual(RNCSTR("1")))
			{
				category |= InputDevice::Category::Mouse;
			}

			classValue = RNSTR(udev_device_get_property_value(child, "ID_INPUT_KEYBOARD"));
			if(classValue && classValue->IsEqual(RNCSTR("1")))
			{
				category |= InputDevice::Category::Keyboard;
			}
        }

        udev_enumerate_unref(enumerate);

        if(category & InputDevice::Category::Keyboard && category & InputDevice::Category::Mouse)
		{
        	category = InputDevice::Category::Mouse;
		}

        return category;
    }

    void LinuxPlatformHIDDeviceAdded(udev_device *device)
    {
        String *path = RNSTR(udev_device_get_devnode(device));
        if(!path || path->GetLength() == 0) return;

        uint32 vendorID = 0;
        uint32 productID = 0;
        const char *vendorIDString = udev_device_get_sysattr_value(device, "idVendor");
        const char *productIDString = udev_device_get_sysattr_value(device, "idProduct");
        if(vendorIDString) vendorID = atoi(vendorIDString);
        if(productIDString) productID = atoi(productIDString);

        String *productName;
        String *vendorName;
        productName = RNSTR(udev_device_get_sysattr_value(device, "manufacturer"));
        vendorName = RNSTR(udev_device_get_sysattr_value(device, "product"));

        InputDevice::Category category = LinuxPlatformGetHIDDeviceCategory(device);

        if(category != 0)
        {
            InputDevice::Descriptor descriptor(category);
            descriptor.SetVendor(vendorName);
            descriptor.SetName(productName);
            descriptor.SetVendorID(Number::WithUint32(vendorID));
            descriptor.SetProductID(Number::WithUint32(productID));

            LinuxPlatformDevice *platformDevice = new LinuxPlatformDevice(descriptor, device);
			platformDevice->Register();
			platformDevice->Activate();

            _foundDevices->AddObject(platformDevice->Autorelease());
        }
    }

    void LinuxPlatformHIDDeviceRemoved(udev_device *deviceRef)
    {
        _foundDevices->Enumerate<LinuxPlatformDevice>([&](LinuxPlatformDevice *device, size_t index, bool &stop) {

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
    }*/

    void BuildPlatformDeviceTree()
    {
        _foundDevices = new Array();
 /*       _udev = udev_new();

        if(!_udev)
        {
            RNDebug("Couldn't create udev instance");
            return;
        }

        _udevMonitor = udev_monitor_new_from_netlink(_udev, "udev");
        if(!_udevMonitor)
        {
            RNDebug("Couldn't create udev monitor instance");
            return;
        }

        udev_monitor_filter_add_match_subsystem_devtype(_udevMonitor, "hidraw", nullptr);
        udev_monitor_enable_receiving(_udevMonitor);


        struct udev_enumerate *enumerate = udev_enumerate_new(_udev);
        udev_enumerate_add_match_subsystem(enumerate, "hidraw");
        udev_enumerate_scan_devices(enumerate);
        struct udev_list_entry *device_entry_list;
        device_entry_list = udev_enumerate_get_list_entry(enumerate);
        struct udev_list_entry *device_entry;
        for(device_entry = device_entry_list; device_entry; device_entry = udev_list_entry_get_next(device_entry))
        {
            const char *path;
            path = udev_list_entry_get_name(device_entry);
            struct udev_device *device;
            device = udev_device_new_from_syspath(_udev, path);

            //RNDebug("Device: " << udev_device_get_devnode(device));

            struct udev_device *usbParent;
            usbParent = udev_device_get_parent_with_subsystem_devtype(device, "usb", "usb_device");

            if(usbParent)
            {
                LinuxPlatformHIDDeviceAdded(usbParent);
                udev_device_unref(usbParent);
            }
        }

        udev_enumerate_unref(enumerate);*/
    }

    void TearDownPlatformDeviceTree()
    {
		_foundDevices->Enumerate<LinuxPlatformDevice>([](LinuxPlatformDevice *device, size_t index, bool &stop) {

			if(device->IsActive())
				device->Deactivate();

			if(device->IsRegistered())
				device->Unregister();
		});

		_foundDevices->Release();
		_foundDevices = nullptr;

 //       udev_unref(_udev);
 //       _udev = nullptr;
    }
}
