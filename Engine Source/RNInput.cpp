//
//  RNInput.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNInput.h"

namespace RN
{
	const char *KeyboarButtonNames[232] =
	{
		"0x00", "0x01", "0x02", "0x03", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",
		"M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "1", "2",
		"3", "4", "5", "6", "7", "8", "9", "0", "Return", "Escape", "Delete", "Tab", "Space", "-", "=", "[",
		"]", "\\", "Pound", ";", "'", "`", ",", ".", "/", "Caps Lock", "F1", "F2", "F3", "F4", "F5", "F6",
		"F7", "F8", "F9", "F10", "F11", "F12", "Print", "Scroll Lock", "Pause", "Insert", "Home", "Page Up", "Del", "End", "Page Down", "Right",
		"Left", "Down", "Up", "Clear", "Pad /", "Pad *", "Pad -", "Pad +", "Enter", "Pad 1", "Pad 2", "Pad 3", "Pad 4", "Pad 5", "Pad 6", "Pad 7",
		"Pad 8", "Pad 9", "Pad 0", "Pad .", "Slash", "Appl", "Power", "Pad =", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20",
		"F21", "F22", "F23", "F24", "Exec", "Help", "Menu", "Select", "Stop", "Again", "Undo", "Cut", "Copy", "Paste", "Find", "Mute",
		"Vol Up", "Vol Down", "Caps-Lock", "Num-Lock", "Scroll-Lock", "Pad ,", "Pad =", "Inter1", "Inter2", "Inter3", "Inter4", "Inter5", "Inter6", "Inter7", "Inter8", "Inter9",
		"Lang1", "Lang2", "Lang3", "Lang4", "Lang5", "Lang6", "Lang7", "Lang8", "Lang9", "Erase", "SysReq", "Cancel", "Clear", "Prior", "Ret", "Separator",
		"Out", "Oper", "Clear/Again", "CrSel", "ExCel", "0xA5", "0xA6", "0xA7", "0xA8", "0xA9", "0xAA", "0xAB", "0xAC", "0xAD", "0xAE", "0xAF",
		"0xB0", "0xB1", "0xB2", "0xB3", "0xB4", "0xB5", "0xB6", "0xB7", "0xB8", "0xB9", "0xBA", "0xBB", "0xBC", "0xBD", "0xBE", "0xBF",
		"0xC0", "0xC1", "0xC2", "0xC3", "0xC4", "0xC5", "0xC6", "0xC7", "0xC8", "0xC9", "0xCA", "0xCB", "0xCC", "0xCD", "0xCE", "0xCF",
		"0xD0", "0xD1", "0xD2", "0xD3", "0xD4", "0xD5", "0xD6", "0xD7", "0xD8", "0xD9", "0xDA", "0xDB", "0xDC", "0xDD", "0xDE", "0xDF",
		"Left Control", "Left Shift", "Left Option", "Left Command", "Right Control", "Right Shift", "Right Option", "Right Command"
	};
	
	const char KeyboardCharacters[232] =
	{
		'\0', '\0', '\0', '\0', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
		'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
		'3', '4', '5', '6', '7', '8', '9', '0', '\0', '\0', '\0', '\0', ' ', '-', '=', '[',
		']', '\\', '\0', ';', '\'', '`', ',', '.', '/', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
		'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
	};
	
	InputControl::InputControl()
	{
		_cookie = 0;
		_parent = _next = _child = 0;
	}
	
	InputControl::InputControl(IOHIDElementCookie cookie, const std::string& name) :
		_name(name),
		_cookie(cookie)
	{
		_parent = _next = _child = 0;
	}
	
	InputControl *InputControl::NextControl() const
	{
		if(_child)
			return _child;
		
		if(_next)
			return _next;
		
		if(_parent)
			return _parent->_next;
		
		return _parent;
	}
	
	void InputControl::AddChild(InputControl *control)
	{
		control->_next = _child;
		control->_parent = this;
		
		_child = control;
	}
	
	void InputControl::AddControl(InputControl *control)
	{
		control->_next = _next;
		control->_parent = _parent;
		
		_next = control;
	}
	
	
	KeyboardControl::KeyboardControl(IOHIDElementCookie cookie, const std::string& name, char character) :
		InputControl(cookie, name)
	{
		_character = character;
	}
	
	
	InputDevice::InputDevice(InputDeviceType type, io_object_t object, CFMutableDictionaryRef properties) :
		_type(type)
	{
		_pluginInterface = 0;
		_deviceInterface = 0;
		_deviceQueue = 0;
		_active = false;
		
		const void *productValue = CFDictionaryGetValue(properties, CFSTR(kIOHIDProductKey));
		if(productValue)
		{
			SInt32 score;
			char name[255];
			
			CFStringGetCString((CFStringRef)productValue, name, 255, kCFStringEncodingUTF8);
			_name = std::string(name);
			
			if(IOCreatePlugInInterfaceForService(object, kIOHIDDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &_pluginInterface, &score) == kIOReturnSuccess)
			{
				if((**_pluginInterface).QueryInterface(_pluginInterface, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID), (void **)&_deviceInterface) == S_OK)
				{
					(**_deviceInterface).open(_deviceInterface, 0);
					_deviceQueue = (**_deviceInterface).allocQueue(_deviceInterface);
					
					if(_deviceQueue && (**_deviceQueue).create(_deviceQueue, 0, 1024) == kIOReturnSuccess)
					{
						printf("Device name: %s, type: %i\n", name, (int)type);
						BuildControlTree(this, properties);
					}
				}
			}
			
			
		}
	}
	
	InputDevice::~InputDevice()
	{
		if(_deviceQueue)
		{
			(**_deviceQueue).dispose(_deviceQueue);
			(**_deviceQueue).Release(_deviceQueue);
		}
		
		if(_deviceInterface)
		{
			(**_deviceInterface).close(_deviceInterface);
			(**_deviceInterface).Release(_deviceInterface);
		}
		
		if(_pluginInterface)
			IODestroyPlugInInterface(_pluginInterface);
	}
	
	void InputDevice::BuildControlTree(InputControl *control, CFMutableDictionaryRef properties)
	{
		CFArrayRef elementArray = (CFArrayRef)CFDictionaryGetValue(properties, CFSTR(kIOHIDElementKey));
		if(elementArray && CFGetTypeID(elementArray) == CFArrayGetTypeID())
		{
			CFIndex count = CFArrayGetCount(elementArray);
			
			for(CFIndex i=0; i<count; i++)
			{
				CFMutableDictionaryRef elementProperties = (CFMutableDictionaryRef)CFArrayGetValueAtIndex(elementArray, i);
			
				if(elementProperties && CFGetTypeID(elementProperties) == CFDictionaryGetTypeID())
				{
					CFNumberRef elementTypeValue = (CFNumberRef)CFDictionaryGetValue(elementProperties, CFSTR(kIOHIDElementTypeKey));
					if(elementTypeValue)
					{
						int32 elementType;
						CFNumberGetValue(elementTypeValue, kCFNumberSInt32Type, &elementType);
						
						switch(elementType)
						{
							case kIOHIDElementTypeCollection:
							{
								InputControl *collection = new InputControl();
								
								control->AddChild(collection);
								BuildControlTree(collection, elementProperties);
								break;
							}
							
							case kIOHIDElementTypeInput_Misc:
							case kIOHIDElementTypeInput_Button:
							case kIOHIDElementTypeInput_Axis:
							case kIOHIDElementTypeInput_ScanCodes:
							{
								CFNumberRef cookieValue = (CFNumberRef)CFDictionaryGetValue(elementProperties, CFSTR(kIOHIDElementCookieKey));
								if(cookieValue)
								{
									int32 cookie;
									CFNumberGetValue(cookieValue, kCFNumberSInt32Type, &cookie);
									
									CFNumberRef usagePageValue = (CFNumberRef)CFDictionaryGetValue(elementProperties, CFSTR(kIOHIDElementUsagePageKey));
									CFNumberRef usageValue = (CFNumberRef)CFDictionaryGetValue(elementProperties, CFSTR(kIOHIDElementUsageKey));
								
									if(usagePageValue && usageValue)
									{
										int32 usagePage;
										int32 usage;
										
										CFNumberGetValue(usagePageValue, kCFNumberSInt32Type, &usagePage);
										CFNumberGetValue(usageValue, kCFNumberSInt32Type, &usage);
										
										if(usagePage == kHIDPage_KeyboardOrKeypad)
										{
											bool valid = ((uint32)(usage - 0xE0) < 8U);
											if(valid)
											{
												CFBooleanRef arrayValue = (CFBooleanRef)CFDictionaryGetValue(elementProperties, CFSTR(kIOHIDElementIsArrayKey));
												if(arrayValue && CFBooleanGetValue(arrayValue))
													valid = false;
											}
											else
											{
												valid = ((uint32)(usage - 4) < 100U);
											}
											
											if(valid)
											{
												const char *name = KeyboarButtonNames[usage];
												char character = KeyboardCharacters[usage];
												
												KeyboardControl *key = new KeyboardControl((IOHIDElementCookie)cookie, std::string(name), character);
												control->AddControl(key);
											}
										}
									}
								}
								
								break;
							}
								
							default:
								break;
						};
					}
				}
			}
		}
	}
	
	void InputDevice::Activate()
	{
		if(!_active)
		{
			_active = true;
			
			(**_deviceQueue).start(_deviceQueue);
			
			InputControl *control = FirstControl();
			while(control)
			{
				(**_deviceQueue).addElement(_deviceQueue, control->Cookie(), 0);
				control = control->NextControl();
			}
		}
	}
	
	void InputDevice::Deactivate()
	{
		if(_active)
		{
			(**_deviceQueue).stop(_deviceQueue);
			
			InputControl *control = FirstControl();
			while(control)
			{
				(**_deviceQueue).removeElement(_deviceQueue, control->Cookie());
				control = control->NextControl();
			}
			
			_active = false;
		}
	}
	
	void InputDevice::DispatchInputEvents()
	{
		static const AbsoluteTime zero = { 0, 0 };
		
		IOHIDEventStruct event;
		IOHIDElementCookie cookie;
		
		std::vector<InputControl *> newControls;
		
		while((**_deviceQueue).getNextEvent(_deviceQueue, &event, zero, 0) == kIOReturnSuccess)
		{
			cookie = event.elementCookie;
			
			InputControl *control = FirstControl();
			while(control)
			{
				if(control->Cookie() == cookie)
				{
					switch(event.value)
					{
						case 0:
						{
							for(auto i=_pressedControls.begin(); i!=_pressedControls.end(); i++)
							{
								InputControl *temp = *i;
								if(temp == control)
								{
									InputMessage message = InputMessage(control, InputMessage::InputMessageTypeKeyUp);
									MessageCenter::SharedInstance()->PostMessage(&message);
									
									_pressedControls.erase(i);
									break;
								}
							}
							
							break;
						}
							
						case 1:
						{
							newControls.push_back(control);
							
							InputMessage message = InputMessage(control, InputMessage::InputMessageTypeKeyDown);
							MessageCenter::SharedInstance()->PostMessage(&message);
							break;
						}
							
						default:
							break;
					}
				}
				
				control = control->NextControl();
			}
		}
		
		for(auto i=_pressedControls.begin(); i!=_pressedControls.end(); i++)
		{
			InputMessage message = InputMessage(*i, InputMessage::InputMessageTypeKeyPressed);
			MessageCenter::SharedInstance()->PostMessage(&message);
		}
		
		_pressedControls.insert(_pressedControls.end(), newControls.begin(), newControls.end());
	}
	
	
	InputMessage::InputMessage(InputControl *control, MessageSubgroup subgroup) :
		Message(MessageGroupInput, subgroup)
	{
		_control = control;
		
		character = control->Character();
		isKeyboard = (subgroup & InputMessageTypeKeyDown || subgroup & InputMessageTypeKeyPressed || subgroup & InputMessageTypeKeyUp);
	}
	
	
	Input::Input()
	{
		ReadInputDevices();
	}
	
	Input::~Input()
	{
		for(auto i=_devices.begin(); i!=_devices.end(); i++)
		{
			InputDevice *device = *i;
			delete device;
		}
	}
	
	void Input::ReadInputDevices()
	{
		io_iterator_t iterator;
		io_object_t object;
		
		CFMutableDictionaryRef dictionary = IOServiceMatching(kIOHIDDeviceKey);
		IOServiceGetMatchingServices(kIOMasterPortDefault, dictionary, &iterator);
		
		while((object = IOIteratorNext(iterator)))
		{
			CFMutableDictionaryRef properties;
			if(IORegistryEntryCreateCFProperties(object, &properties, kCFAllocatorDefault, kNilOptions) == KERN_SUCCESS)
			{
				const void *usagePageValue = CFDictionaryGetValue(properties, CFSTR(kIOHIDPrimaryUsagePageKey));
				const void *usageValue = CFDictionaryGetValue(properties, CFSTR(kIOHIDPrimaryUsageKey));
				
				if(usagePageValue && usageValue)
				{
					int32 usagePage;
					int32 usage;
					
					CFNumberGetValue((CFNumberRef)usagePageValue, kCFNumberSInt32Type, &usagePage);
					CFNumberGetValue((CFNumberRef)usageValue, kCFNumberSInt32Type, &usage);
					
					if(usagePage == kHIDPage_GenericDesktop)
					{
						switch(usage)
						{
							case kHIDUsage_GD_Keyboard:
							case kHIDUsage_GD_Keypad:
							{
								InputDevice *device = new InputDevice(InputDevice::InputDeviceKeyboard, object, properties);
								_devices.push_back(device);
								break;
							}
								
							case kHIDUsage_GD_Joystick:
							case kHIDUsage_GD_GamePad:
							{
								InputDevice *device = new InputDevice(InputDevice::InputDeviceGamepad, object, properties);
								_devices.push_back(device);
								break;
							}
								
							default:
								break;
						}
					}
				}
				
				CFRelease(properties);
			}
			
			IOObjectRelease(object);
		}
		
		IOObjectRelease(iterator);
	}
	
	void Input::DispatchInputEvents()
	{
		for(auto i=_devices.begin(); i!=_devices.end(); i++)
		{
			InputDevice *device = *i;
			if(device->IsActive())
			{
				device->DispatchInputEvents();
			}
		}
	}
	
	void Input::Activate()
	{
		for(auto i=_devices.begin(); i!=_devices.end(); i++)
		{
			InputDevice *device = *i;
			device->Activate();
		}
	}
	
	void Input::Deactivate()
	{
		for(auto i=_devices.begin(); i!=_devices.end(); i++)
		{
			InputDevice *device = *i;
			device->Deactivate();
		}
	}
	
#if RN_PLATFORM_MAC_OS
	void Input::HandleKeyboardEvent(NSEvent *event)
	{
	}
	
	void Input::HandleMouseEvent(NSEvent *event)
	{
	}
#endif
}
