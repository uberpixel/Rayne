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
#include "RNVector.h"

namespace RN
{
	
#pragma mark -
#pragma mark Input controller
	
	class InputDevice;
	class InputControl
	{
	friend class InputDevice;
	public:
		typedef enum
		{
			InputControlTypeGroup,
			InputControlTypeKeyboard,
			InputControlTypeDeltaAxis,
			InputControlTypeMouseButton
		} InputControlType;
		
		InputControl(InputDevice *device);
		InputControl(InputControlType type, InputDevice *device, IOHIDElementCookie cookie, const std::string& name);
		
		InputControl *FirstControl() const;
		InputControl *NextControl() const;
		
		InputDevice *Device() const { return _device; }
		InputControlType Type() const { return _type; }
		
		IOHIDElementCookie Cookie() const { return _cookie; }
		const std::string& Name() const { return _name; }
		
	protected:
		void AddControl(InputControl *control);
		void AddChild(InputControl *control);
		
		IOHIDElementCookie _cookie;
		std::string _name;
		
	private:
		InputDevice *_device;
		InputControlType _type;
		
		InputControl *_parent;
		InputControl *_child;
		InputControl *_next;
	};
	
	class KeyboardControl : public InputControl
	{
	public:
		KeyboardControl(InputDevice *device, IOHIDElementCookie cookie, const std::string& name, char character);
		
		char Character() const { return _character; }
		
	private:
		char _character;
	};
	
	class DeltaAxisControl : public InputControl
	{
	public:
		DeltaAxisControl(InputDevice *device, IOHIDElementCookie cookie, const std::string& name, uint32 axis);
		
		uint32 Axis() const { return _axis; }
		
	private:
		uint32 _axis;
	};
	
	class MouseButtonControl : public InputControl
	{
	public:
		MouseButtonControl(InputDevice *device, IOHIDElementCookie cookie, const std::string& name, uint32 button);
		
		uint32 Button() const { return _button; }
		
	private:
		uint32 _button;
	};
	
#pragma mark -
#pragma mark Input Devices
	
	class InputDevice
	{
	public:
		typedef enum
		{
			InputDeviceTypeMouse,
			InputDeviceTypeKeyboard
		} InputDeviceType;
		
		InputDevice(InputDeviceType type, io_object_t object, CFMutableDictionaryRef properties);
		virtual ~InputDevice();
		
		void Activate();
		void Deactivate();
		
		bool IsActive() const { return _active; }
		InputDeviceType Type() const { return _type; }
		const std::string& Name() const { return _name; }
		
		virtual void DispatchInputEvents();
		
	protected:
		IOCFPlugInInterface **_pluginInterface;
		IOHIDDeviceInterface **_deviceInterface;
		IOHIDQueueInterface **_deviceQueue;
		
		InputControl *_root;
		
	private:
		void BuildControlTree(InputControl *control, CFMutableDictionaryRef properties);
		
		InputDeviceType _type;
		std::string _name;
		bool _active;
	};
	
	class InputDeviceMouse : public InputDevice
	{
	public:
		InputDeviceMouse(io_object_t object, CFMutableDictionaryRef properties);
		
		virtual void DispatchInputEvents();
		
		const Vector3& MouseDelta() { return _mouseDelta; }
		uint32 PressedButtons() { return _pressedButtons; }
		
	private:
		Vector3 _mouseDelta;
		uint32 _pressedButtons;
	};
	
	class InputDeviceKeyboard : public InputDevice
	{
	public:
		InputDeviceKeyboard(io_object_t object, CFMutableDictionaryRef properties);
		
		bool KeyPressed(char key) const;
		virtual void DispatchInputEvents();
		
	private:
		std::vector<InputControl *> _pressedControls;
		std::unordered_set<char> _pressedCharacters;
	};
	
	
#pragma mark -
#pragma mark Misc
	
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
		uint32 button;
		uint32 axis;
		
		bool isKeyboard;
		bool isMouse;
		
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
		
		void UpdateInputDevices();
		
#if RN_PLATFORM_MAC_OS
		void HandleKeyboardEvent(NSEvent *event);
		void HandleMouseEvent(NSEvent *event);
#endif
		
		const Vector3& MouseDelta() { return _mouseDelta; }
		uint32 PressedButtons() { return _pressedButtons; }
		bool KeyPressed(char key) const;
		
	private:
		void ReadInputDevices();
		
		Vector3 _mouseDelta;
		uint32 _pressedButtons;
		
		std::vector<InputDevice *> _devices;
	};
}

#endif /* __RAYNE_INPUT_H__ */
