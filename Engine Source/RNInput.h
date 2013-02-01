//
//  RNInput.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INPUT_H__
#define __RAYNE_INPUT_H__

#include "RNBase.h"
#include "RNMessage.h"

namespace RN
{
	class InputControl
	{
	public:
		InputControl();
		InputControl(IOHIDElementCookie cookie, const std::string& name);
		
		InputControl *FirstControl() const { return _child; }
		InputControl *NextControl() const;
		
		IOHIDElementCookie Cookie() const { return _cookie; }
		const std::string& Name() const { return _name; }
		
		virtual char Character() const { return '\0'; }
		
		void AddControl(InputControl *control);
		void AddChild(InputControl *control);
		
	private:
		IOHIDElementCookie _cookie;
		std::string _name;
		
		InputControl *_parent;
		InputControl *_child;
		InputControl *_next;
	};
	
	class KeyboardControl : public InputControl
	{
	public:
		KeyboardControl(IOHIDElementCookie cookie, const std::string& name, char character);
		
		virtual char Character() const { return _character; }
		
	private:
		char _character;
	};
	
	class InputDevice : InputControl
	{
	public:
		typedef enum
		{
			InputDevicePointer,
			InputDeviceKeyboard,
			InputDeviceGamepad
		} InputDeviceType;
		
		InputDevice(InputDeviceType type, io_object_t object, CFMutableDictionaryRef properties);
		~InputDevice();
		
		bool IsActive() const { return _active; }
		void Activate();
		void Deactivate();
		
		void DispatchInputEvents();
		
	private:
		void BuildControlTree(InputControl *control, CFMutableDictionaryRef properties);
		
		IOCFPlugInInterface **_pluginInterface;
		IOHIDDeviceInterface **_deviceInterface;
		IOHIDQueueInterface **_deviceQueue;
		
		InputDeviceType _type;
		std::string _name;
		bool _active;
		
		std::vector<InputControl *> _pressedControls;
	};
	
	class InputMessage : public Message
	{
	public:
		enum
		{
			InputMessageTypeKeyDown = (1 << 0),
			InputMessageTypeKeyUp = (1 << 1),
			InputMessageTypeKeyPressed = (1 << 2)
		};
		
		InputMessage(InputControl *control, MessageSubgroup subgroup);
		
		char character;
		bool isKeyboard;
		
	private:
		InputControl *_control;
	};
	
	class Input : public Singleton<Input>
	{
	public:
		Input();
		~Input();
		
		void DispatchInputEvents();
		
		void Activate();
		void Deactivate();
		
#if RN_PLATFORM_MAC_OS
		void HandleKeyboardEvent(NSEvent *event);
		void HandleMouseEvent(NSEvent *event);
#endif
		
	private:
		void ReadInputDevices();
		
		std::vector<InputDevice *> _devices;
	};
}

#endif /* __RAYNE_INPUT_H__ */
