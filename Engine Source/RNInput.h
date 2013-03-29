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

#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS || RN_PLATFORM_LINUX

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

		RNAPI InputControl(InputDevice *device);

#if RN_PLATFORM_MAC_OS
		RNAPI InputControl(InputControlType type, InputDevice *device, IOHIDElementCookie cookie, const std::string& name);
#endif

		RNAPI InputControl *FirstControl() const;
		RNAPI InputControl *NextControl() const;

		InputDevice *Device() const { return _device; }
		InputControlType Type() const { return _type; }

#if RN_PLATFORM_MAC_OS
		IOHIDElementCookie Cookie() const { return _cookie; }
#endif

		const std::string& Name() const { return _name; }

	protected:
		void AddControl(InputControl *control);
		void AddChild(InputControl *control);

#if RN_PLATFORM_MAC_OS
		IOHIDElementCookie _cookie;
#endif

		std::string _name;

	private:
		InputDevice *_device;
		InputControlType _type;

		InputControl *_parent;
		InputControl *_child;
		InputControl *_next;
	};

#if RN_PLATFORM_MAC_OS
	class KeyboardControl : public InputControl
	{
	public:
		RNAPI KeyboardControl(InputDevice *device, IOHIDElementCookie cookie, const std::string& name, char character);

		char Character() const { return _character; }

	private:
		char _character;
	};

	class DeltaAxisControl : public InputControl
	{
	public:
		RNAPI DeltaAxisControl(InputDevice *device, IOHIDElementCookie cookie, const std::string& name, uint32 axis);

		uint32 Axis() const { return _axis; }

	private:
		uint32 _axis;
	};

	class MouseButtonControl : public InputControl
	{
	public:
		RNAPI MouseButtonControl(InputDevice *device, IOHIDElementCookie cookie, const std::string& name, uint32 button);

		uint32 Button() const { return _button; }

	private:
		uint32 _button;
	};
#endif

#endif

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

#if RN_PLATFORM_MAC_OS
		RNAPI InputDevice(InputDeviceType type, io_object_t object, CFMutableDictionaryRef properties);
#endif
#if RN_PLATFORM_IOS
		RNAPI InputDevice(InputDeviceType type, const std::string& name);
#endif

		RNAPI virtual ~InputDevice();

		RNAPI virtual void Activate();
		RNAPI virtual void Deactivate();

		bool IsActive() const { return _active; }

		InputDeviceType Type() const { return _type; }
		const std::string& Name() const { return _name; }

		virtual void DispatchInputEvents();

	protected:
#if RN_PLATFORM_MAC_OS
		IOCFPlugInInterface **_pluginInterface;
		IOHIDDeviceInterface **_deviceInterface;
		IOHIDQueueInterface **_deviceQueue;
#endif

#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
		InputControl *_root;
#endif

	private:
#if RN_PLATFORM_MAC_OS
		void BuildControlTree(InputControl *control, CFMutableDictionaryRef properties);
#endif

		InputDeviceType _type;
		std::string _name;
		bool _active;
	};

#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS 

	class InputDeviceMouse : public InputDevice
	{
	public:
		RNAPI InputDeviceMouse(io_object_t object, CFMutableDictionaryRef properties);

		RNAPI virtual void DispatchInputEvents();

		const Vector3& MouseDelta() { return _mouseDelta; }
		uint32 PressedButtons() { return _pressedButtons; }

	private:
		Vector3 _mouseDelta;
		uint32 _pressedButtons;
	};

	class InputDeviceKeyboard : public InputDevice
	{
	public:
		RNAPI InputDeviceKeyboard(io_object_t object, CFMutableDictionaryRef properties);

		RNAPI bool KeyPressed(char key) const;
		RNAPI virtual void DispatchInputEvents();

	private:
		std::unordered_set<char> _pressedCharacters;
	};

#endif


#pragma mark -
#pragma mark Misc

#if RN_PLATFORM_IOS
	struct Touch
	{
		typedef enum
		{
			TouchPhaseBegan,
			TouchPhaseMoved,
			TouchPhaseEnded,
			TouchPhaseCancelled
		} TouchPhase;

		Vector2 location;
		Vector2 initialLocation;
		Vector2 previousLocation;
		Vector2 deltaLocation;

		TouchPhase phase;
		uint32 uniqueID;
		bool changed;
		void *userData;
	};
#endif

	class InputMessage : public Message
	{
	public:
		enum
		{
			InputMessageTypeKeyDown = (1 << 0),
			InputMessageTypeKeyUp = (1 << 1),

			InputMessageTypeMouseDown = (1 << 2),
			InputMessageTypeMouseUp = (1 << 3),

			InputMessageTypeTouchDown = (1 << 4),
			InputMessageTypeTouchMoved = (1 << 5),
			InputMessageTypeTouchUp = (1 << 6),
			InputMessageTypeTouchCancelled = (1 << 7)
		};

#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS || RN_PLATFORM_LINUX
		RNAPI InputMessage(InputControl *control, MessageSubgroup subgroup);
#endif
#if RN_PLATFORM_IOS
		RNAPI InputMessage(InputDevice *device, MessageSubgroup subgroup);
		RNAPI InputMessage(Touch *touch);

		Touch *touch;
#endif

		char character;
		uint32 button;
		uint32 axis;

		bool isKeyboard;
		bool isMouse;
		bool isTouch;

	private:
		void Initialize();

#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS || RN_PLATFORM_LINUX
		InputControl *_control;
#endif
#if RN_PLATFORM_IOS
		InputDevice *_device;
#endif
	};

	class Input : public Singleton<Input>
	{
	friend class Window;
	public:
		RNAPI Input();
		RNAPI ~Input();

		RNAPI void DispatchInputEvents();

		RNAPI void Activate();
		RNAPI void Deactivate();


#if RN_PLATFORM_IOS
		RNAPI void HandleTouchEvent(const Touch& touch);
		const std::vector<Touch>& Touches() const { return _touches; }
#endif

		const Vector3& MouseDelta() { return _mouseDelta; }
		uint32 PressedButtons() { return _pressedButtons; }
		RNAPI bool KeyPressed(char key) const;

	private:
#if RN_PLATFORM_IOS
		void DispatchTouchEvent(const Touch& input);
#endif

		void ReadInputDevices();

		Vector3 _mouseDelta;
		uint32 _pressedButtons;

		std::vector<InputDevice *> _devices;

#if RN_PLATFORM_IOS
		uint32 _touchID;

		std::vector<Touch> _queuedTouchEvents;
		std::vector<Touch> _touches;
#endif

#if RN_PLATFORM_LINUX
		bool _mouseEntered;
		Vector3 _lastMouseAbs;

		void HandleXInputEvents(XEvent *event);
		
		std::unordered_set<char> _pressedCharacters;
#endif
	};
}

#endif /* __RAYNE_INPUT_H__ */
