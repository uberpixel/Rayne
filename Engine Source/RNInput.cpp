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
	
#pragma mark -
#pragma mark Input controller
	
#if RN_PLATFORM_MAC_OS
	const char *KeyboardButtonNames[232] =
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
		']', '\\', '#', ';', '\'', '`', ',', '.', '/', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
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
	
	const char *DeltaAxisNames[3] =
	{
		"X-Delta ", "Y-Delta ", "Z-Delta "
	};
#endif
	
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
	
	InputControl::InputControl(InputDevice *device) :
		_device(device)
	{
		_cookie = 0;
		_type = InputControlTypeGroup;
		_parent = _next = _child = 0;
	}
	
#if RN_PLATFORM_MAC_OS
	InputControl::InputControl(InputControlType type, InputDevice *device, IOHIDElementCookie cookie, const std::string& name) :
		_name(name),
		_cookie(cookie),
		_type(type),
		_device(device)
	{
		_parent = _next = _child = 0;
	}
#endif
	
	InputControl *InputControl::FirstControl() const
	{
		return _child;
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
	
	
	KeyboardControl::KeyboardControl(InputDevice *device, IOHIDElementCookie cookie, const std::string& name, char character) :
		InputControl(InputControlTypeKeyboard, device, cookie, name)
	{
		_character = character;
	}
	
	DeltaAxisControl::DeltaAxisControl(InputDevice *device, IOHIDElementCookie cookie, const std::string& name, uint32 axis) :
		InputControl(InputControlTypeDeltaAxis, device, cookie, name)
	{
		_axis = axis;
	}
	
	MouseButtonControl::MouseButtonControl(InputDevice *device, IOHIDElementCookie cookie, const std::string& name, uint32 button) :
		InputControl(InputControlTypeMouseButton, device, cookie, name)
	{
		_button = button;
	}
	
#endif
	
#pragma mark -
#pragma mark Input devices
	
#if RN_PLATFORM_MAC_OS
	
	InputDevice::InputDevice(InputDeviceType type, io_object_t object, CFMutableDictionaryRef properties) :
		_type(type)
	{
		_pluginInterface = 0;
		_deviceInterface = 0;
		_deviceQueue = 0;
		_active = false;
		_root = new InputControl(this);
		
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
						BuildControlTree(_root, properties);
					}
				}
			}
			
			
		}
	}
	
	InputDevice::~InputDevice()
	{
		delete _root;
		
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
								InputControl *collection = new InputControl(this);
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
									IOHIDElementCookie cookie;
									CFNumberGetValue(cookieValue, kCFNumberSInt32Type, &cookie);
									
									CFNumberRef usagePageValue = (CFNumberRef)CFDictionaryGetValue(elementProperties, CFSTR(kIOHIDElementUsagePageKey));
									CFNumberRef usageValue = (CFNumberRef)CFDictionaryGetValue(elementProperties, CFSTR(kIOHIDElementUsageKey));
								
									if(usagePageValue && usageValue)
									{
										int32 usagePage;
										int32 usage;
										
										CFNumberGetValue(usagePageValue, kCFNumberSInt32Type, &usagePage);
										CFNumberGetValue(usageValue, kCFNumberSInt32Type, &usage);
										
										InputControl *elementControl = 0;
										
										if(usagePage == kHIDPage_GenericDesktop)
										{
											switch(usage)
											{
												case kHIDUsage_GD_X:
												case kHIDUsage_GD_Y:
												case kHIDUsage_GD_Z:
												{
													bool relative = false;
													int axis = usage - kHIDUsage_GD_X;
													
													CFBooleanRef relativeValue = (CFBooleanRef)CFDictionaryGetValue(elementProperties, CFSTR(kIOHIDElementIsRelativeKey));
													if(relativeValue)
														relative = CFBooleanGetValue(relativeValue);
													
													if(relative)
													{
														const char *name = DeltaAxisNames[axis];
														elementControl = new DeltaAxisControl(this, cookie, std::string(name), axis);
													}
													
													break;
												}
													
												case kHIDUsage_GD_Wheel:
												{
													uint32 axis = 2;
													
													const char *name = DeltaAxisNames[axis];
													elementControl = new DeltaAxisControl(this, cookie, std::string(name), axis);
													
													break;
												}
											}
										}
										else
										if(usagePage == kHIDPage_Button)
										{
											char buffer[32];
											sprintf(buffer, "Button %i", usage);
											
											elementControl = new MouseButtonControl(this, cookie, std::string(buffer), usage);
										}
										else
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
												const char *name = KeyboardButtonNames[usage];
												char character = KeyboardCharacters[usage];
												
												elementControl = new KeyboardControl(this, cookie, std::string(name), character);
											}
										}
										
										if(elementControl)
										{
											control->AddControl(elementControl);
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
			
			InputControl *control = _root->FirstControl();
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
			
			InputControl *control = _root->FirstControl();
			while(control)
			{
				(**_deviceQueue).removeElement(_deviceQueue, control->Cookie());
				control = control->NextControl();
			}
			
			_active = false;
		}
	}
	
#endif
	
#if RN_PLATFORM_IOS
	
	InputDevice::InputDevice(InputDeviceType type, const std::string& name) :
		_type(type),
		_name(name)
	{
		_active = false;
	}
	
	InputDevice::~InputDevice()
	{
	}
	
	void InputDevice::Activate()
	{
		_active = true;
	}
	
	void InputDevice::Deactivate()
	{
		_active = false;
	}
	
#endif
	
	void InputDevice::DispatchInputEvents()
	{
	}
	
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
	
	InputDeviceMouse::InputDeviceMouse(io_object_t object, CFMutableDictionaryRef properties) :
		InputDevice(InputDeviceTypeMouse, object, properties)
	{
	}
	
	void InputDeviceMouse::DispatchInputEvents()
	{
		static const AbsoluteTime zero = { 0, 0 };
		
		IOHIDEventStruct event;
		IOHIDElementCookie cookie;
		
		std::vector<InputControl *> newControls;
		
		_mouseDelta = Vector3();
		
		while((**_deviceQueue).getNextEvent(_deviceQueue, &event, zero, 0) == kIOReturnSuccess)
		{
			cookie = event.elementCookie;
			
			InputControl *control = _root->FirstControl();
			while(control)
			{
				if(control->Cookie() == cookie)
				{
					switch(control->Type())
					{
						case InputControl::InputControlTypeDeltaAxis:
						{
							DeltaAxisControl *tcontrol = (DeltaAxisControl *)control;
							float value = (float)event.value;
							
							switch(tcontrol->Axis())
							{
								case 0:
									_mouseDelta.x += value;
									break;
									
								case 1:
									_mouseDelta.y += value;
									break;
									
								case 2:
									_mouseDelta.z += value;
									break;
									
								default:
									break;
							}
							
							break;
						}
							
						case InputControl::InputControlTypeMouseButton:
						{
							MouseButtonControl *tcontrol = (MouseButtonControl *)control;
							
							switch(event.value)
							{
								case 0:
								{
									InputMessage message = InputMessage(control, InputMessage::InputMessageTypeMouseUp);
									MessageCenter::SharedInstance()->PostMessage(&message);
									
									_pressedButtons &= ~(1 << tcontrol->Button());
									break;
								}
									
								case 1:
								{
									InputMessage message = InputMessage(control, InputMessage::InputMessageTypeMouseDown);
									MessageCenter::SharedInstance()->PostMessage(&message);
									
									_pressedButtons |= (1 << tcontrol->Button());
									break;
								}
									
								default:
									break;
							}
							
							break;
						}
							
						default:
							break;
					}
					
					
				}
				
				control = control->NextControl();
			}
		}
	}
	
	
	InputDeviceKeyboard::InputDeviceKeyboard(io_object_t object, CFMutableDictionaryRef properties) :
		InputDevice(InputDeviceTypeKeyboard, object, properties)
	{
	}
	
	bool InputDeviceKeyboard::KeyPressed(char key) const
	{
		return (_pressedCharacters.find(key) != _pressedCharacters.end());
	}
	
	void InputDeviceKeyboard::DispatchInputEvents()
	{
		static const AbsoluteTime zero = { 0, 0 };
		
		IOHIDEventStruct event;
		IOHIDElementCookie cookie;
		
		while((**_deviceQueue).getNextEvent(_deviceQueue, &event, zero, 0) == kIOReturnSuccess)
		{
			cookie = event.elementCookie;
			
			InputControl *control = _root->FirstControl();
			while(control)
			{
				if(control->Cookie() == cookie)
				{
					KeyboardControl *keyControl = (KeyboardControl *)control;
					
					switch(event.value)
					{
						case 0:
						{
							InputMessage message = InputMessage(control, InputMessage::InputMessageTypeKeyUp);
							MessageCenter::SharedInstance()->PostMessage(&message);
							
							if(keyControl->Character() != '\0')
								_pressedCharacters.erase(keyControl->Character());
							
							break;
						}
							
						case 1:
						{
							if(keyControl->Character() != '\0')
								_pressedCharacters.insert(keyControl->Character());
							
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
	}
	
#endif
	
	
#pragma mark -
#pragma mark Misc
	
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
	
	InputMessage::InputMessage(InputControl *control, MessageSubgroup subgroup) :
		Message(MessageGroupInput, subgroup)
	{
		Initialize();
		_control = control;
		
		switch(control->Type())
		{
			case InputControl::InputControlTypeKeyboard:
			{
				KeyboardControl *tcontrol = (KeyboardControl *)control;
				
				character = tcontrol->Character();
				isKeyboard = true;
				
				break;
			}
				
			case InputControl::InputControlTypeDeltaAxis:
			{
				DeltaAxisControl *tcontrol = (DeltaAxisControl *)control;
				
				axis = tcontrol->Axis();
				isMouse = true;
				break;
			}
				
			case InputControl::InputControlTypeMouseButton:
			{
				MouseButtonControl *tcontrol = (MouseButtonControl *)control;
				
				button = tcontrol->Button();
				isMouse = true;
				break;
			}
				
			default:
				break;
		}
	}
	
#endif
	
#if RN_PLATFORM_IOS
	
	InputMessage::InputMessage(InputDevice *device, MessageSubgroup subgroup) :
		Message(MessageGroupInput, subgroup)
	{
		Initialize();
		_device = device;
	}
	
	InputMessage::InputMessage(Touch *_touch) :
		Message(MessageGroupInput, 0)
	{
		Initialize();
		touch = _touch;
		
		switch(touch->phase)
		{
			case Touch::TouchPhaseBegan:
				_subgroup = InputMessageTypeTouchDown;
				break;
				
			case Touch::TouchPhaseMoved:
				_subgroup = InputMessageTypeTouchMoved;
				break;
				
			case Touch::TouchPhaseEnded:
				_subgroup = InputMessageTypeTouchUp;
				break;
				
			case Touch::TouchPhaseCancelled:
				_subgroup = InputMessageTypeTouchCancelled;
				break;
				
			default:
				break;
		}
	}
	
#endif
	
	void InputMessage::Initialize()
	{
		character = '\0';
		button = 0;
		axis = -1;
		
#if RN_PLATFORM_IOS
		touch = 0;
		_device = 0;
#endif
		
#if RN_PLATFORM_MAC_OS
		_control = 0;
#endif
		
		isKeyboard = false;
		isMouse = false;
		isTouch = false;
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
	
#if RN_PLATFORM_MAC_OS
	
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
							case kHIDUsage_GD_Mouse:
							case kHIDUsage_GD_Pointer:
							{
								InputDevice *device = new InputDeviceMouse(object, properties);
								_devices.push_back(device);
								break;
							}
								
							case kHIDUsage_GD_Keyboard:
							case kHIDUsage_GD_Keypad:
							{
								InputDevice *device = new InputDeviceKeyboard(object, properties);
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
		_mouseDelta = Vector3();
		_pressedButtons = 0;
		
		for(auto i=_devices.begin(); i!=_devices.end(); i++)
		{
			InputDevice *device = *i;
			if(device->IsActive())
			{
				device->DispatchInputEvents();
				
				if(device->Type() == InputDevice::InputDeviceTypeMouse)
				{
					InputDeviceMouse *mouse = (InputDeviceMouse *)device;
					
					_mouseDelta += mouse->MouseDelta();
					_pressedButtons |= mouse->PressedButtons();
				}
			}
		}
	}
	
	void Input::HandleKeyboardEvent(NSEvent *event)
	{
	}
	
	void Input::HandleMouseEvent(NSEvent *event)
	{
	}
	
#endif
	
#if RN_PLATFORM_IOS
	
	void Input::ReadInputDevices()
	{
	}
	
	void Input::DispatchInputEvents()
	{
		do
		{
			auto i = _touches.begin();
			while(i != _touches.end())
			{
				if(i->phase == Touch::TouchPhaseEnded || i->phase == Touch::TouchPhaseCancelled)
				{
					i = _touches.erase(i);
					continue;
				}
				
				i->changed = false;
				i ++;
			}
		} while (0);
		
		// Dispatch the queued up touch events
		for(auto i=_queuedTouchEvents.begin(); i!=_queuedTouchEvents.end(); i++)
		{
			
			DispatchTouchEvent(*i);
		}
		
		_queuedTouchEvents.clear();
	}
	
	void Input::HandleTouchEvent(const Touch& touch)
	{
		_queuedTouchEvents.push_back(touch);
	}

	void Input::DispatchTouchEvent(const Touch& input)
	{		
		switch(input.phase)
		{
			case Touch::TouchPhaseBegan:
			{
				Touch touch;
				touch.location = input.location;
				touch.initialLocation = touch.location;
				touch.deltaLocation = Vector2();
				touch.phase = Touch::TouchPhaseBegan;
				touch.userData = 0;
				touch.uniqueID = _touchID ++;
				touch.changed = true;
				
				_touches.push_back(touch);
				
				InputMessage message = InputMessage(&touch);
				MessageCenter::SharedInstance()->PostMessage(&message);
				break;
			}
				
			case Touch::TouchPhaseMoved:
			{
				Vector2 location = input.previousLocation;
				
				for(auto i=_touches.begin(); i!=_touches.end(); i++)
				{
					if(i->location.IsEqual(location, 5.0f))
					{						
						i->location = input.location;
						i->previousLocation = location;
						i->deltaLocation = i->location - i->previousLocation;
						i->phase = Touch::TouchPhaseMoved;
						i->changed = true;
						
						InputMessage message = InputMessage(&*i);
						MessageCenter::SharedInstance()->PostMessage(&message);
						break;
					}
				}
				
				break;
			}
				
			case Touch::TouchPhaseEnded:
			case Touch::TouchPhaseCancelled:
			{
				Vector2 location = input.previousLocation;
				
				for(auto i=_touches.begin(); i!=_touches.end(); i++)
				{
					if(i->location.IsEqual(location, 5.0f))
					{
						i->location = input.location;
						i->previousLocation = location;
						i->deltaLocation = i->location - i->previousLocation;
						i->phase = input.phase;
						i->changed = true;
						
						InputMessage message = InputMessage(&*i);
						MessageCenter::SharedInstance()->PostMessage(&message);
						break;
					}
				}
				
				break;
			}
				
			default:
				break;
		}
	}
	
#endif
	
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
	
	bool Input::KeyPressed(char key) const
	{
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
		for(auto i=_devices.begin(); i!=_devices.end(); i++)
		{
			InputDevice *device = *i;
			
			if(device->IsActive() && device->Type() == InputDevice::InputDeviceTypeKeyboard)
			{
				InputDeviceKeyboard *temp = (InputDeviceKeyboard *)device;
				
				if(temp->KeyPressed(key))
					return true;
			}
		}
#endif
		
		return false;
	}
}
