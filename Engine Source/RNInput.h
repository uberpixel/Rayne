//
//  RNInput.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INPUT_H__
#define __RAYNE_INPUT_H__

#include "RNBase.h"
#include "RNMessage.h"
#include "RNObject.h"
#include "RNString.h"
#include "RNArray.h"
#include "RNSpinLock.h"
#include "RNVector.h"
#include "RNEnum.h"

#define kRNInputInputDeviceRegistered   RNCSTR("kRNInputInputDeviceRegistered")
#define kRNInputInputDeviceUnregistered RNCSTR("kRNInputInputDeviceUnregistered")

namespace RN
{
	class InputDevice;
	class InputControl : public Object
	{
	public:
		friend class InputDevice;
		
		~InputControl() override;
		
		const String *GetName() const { return _name; }
		InputDevice *GetDevice() const { return _device; }
		
	protected:
		InputControl(const String *name);
		
	private:
		String *_name;
		InputDevice *_device;
		
		RNDeclareMeta(InputControl)
	};
	
	class Axis2DControl : public InputControl
	{
	public:
		
	protected:
		Axis2DControl(const String *name);
		
	private:
		RNDeclareMeta(Axis2DControl)
	};
	
	
	
	
	class InputDevice : public Object
	{
	public:
		struct Category : public Enum<uint32>
		{
			Category()
			{}
			
			Category(uint32 value) :
				Enum(value)
			{}
			
			enum
			{
				Mouse = (1 << 0),
				Keyboard = (1 << 1),
				Pen = (1 << 2),
				Gamepad  = (1 << 3),
				Joystick = (1 << 4)
			};
		};
		
		~InputDevice() override;
		
		virtual void Update() = 0;
		virtual void Activate() = 0;
		virtual void Deactivate() = 0;
		
		Category GetCategory() const { return _category; }
		const String *GetName() const { return _name; }
		const String *GetVendor() const { return _vendor; }
		const Array *GetControls() const { return _controls; }
		
		bool SupportsCommand(const String *command) const;
		bool SupportsCommand(const String *command, MetaClass *meta) const;
		
		Object *ExecuteCommand(const String *command, Object *argument);
		
		Array *GetSupportedCommands() const;
		std::vector<MetaClass *> GetSupportArgumentsForCommand(const String *command) const;
		
	protected:
		InputDevice(Category category, const String *vendor, const String *name);
		
		void SetSerialNumber(const String *serialNumber);
		
		void AddControl(InputControl *control);
		void BindCommand(const String *command, std::function<Object * (Object *)> &&action, std::vector<MetaClass *> &&arguments);
		
	private:
		struct ExecutionPort
		{
			ExecutionPort(String *tcommand) :
				command(tcommand->Retain())
			{}
			
			ExecutionPort(const ExecutionPort &other) :
				command(other.command->Retain()),
				supportedArguments(other.supportedArguments),
				action(other.action)
			{}
			
			ExecutionPort &operator =(const ExecutionPort &other)
			{
				command->Autorelease();
				command = other.command->Retain();
				
				supportedArguments = other.supportedArguments;
				action = other.action;
				
				return *this;
			}
			
			~ExecutionPort()
			{
				command->Release();
			}
			
			String *command;
			std::function<Object * (Object *)> action;
			std::vector<MetaClass *> supportedArguments;
		};
		
		const ExecutionPort *GetExecutionPortMatching(const String *command, MetaClass *meta) const;
		
		Category _category;
		String *_name;
		String *_vendor;
		String *_serialNumber;
		Array *_controls;
		
		std::vector<ExecutionPort> _executionPorts;
		
		RNDeclareMeta(InputDevice)
	};
	
	
	class GamepadDevice : public InputDevice
	{
	public:
		bool IsButtonPressed(uint8 button) const { return (_buttons & (1LL << button)); }
		
		const Vector2 &GetAnalog1() const { return _analog1; }
		const Vector2 &GetAnalog2() const { return _analog2; }
		
		float GetTrigger1() const { return _trigger1; }
		float GetTrigger2() const { return _trigger2; }
		
	protected:
		GamepadDevice(Category category, const String *vendor, const String *name);
		
		Vector2 _analog1;
		Vector2 _analog2;
		
		uint64 _buttons;
		
		float _trigger1;
		float _trigger2;
		
		RNDeclareMeta(GamepadDevice)
	};
	
	
	
	
	
#define kRNInputEventMessage RNSTR("kRNInputEventMessage")
	
	enum KeyCodes
	{
#if RN_PLATFORM_MAC_OS
		KeyReturn = 0xD,
		
		KeyF1 = 0xF704,
		KeyF2,
		KeyF3,
		KeyF4,
		KeyF5,
		KeyF6,
		KeyF7,
		KeyF8,
		KeyF9,
		KeyF10,
		KeyF11,
		KeyF12,
		
		KeyUp = 0xF700,
		KeyDown,
		KeyLeft,
		KeyRight,
		
		KeyDelete = 0x07F,
		KeyBegin = 0xF72A,
		KeyEnd,
		
		KeyPageUp = 0xF72C,
		KeyPageDown,
		
		KeyPrintScreen = 0xF72E,
		KeyESC = 0x1B
#endif
#if RN_PLATFORM_WINDOWS
		KeyReturn = 0xD,

		KeyF1 = 0xF770,
		KeyF2,
		KeyF3,
		KeyF4,
		KeyF5,
		KeyF6,
		KeyF7,
		KeyF8,
		KeyF9,
		KeyF10,
		KeyF11,
		KeyF12,

		KeyUp = 0xF726,
		KeyDown = 0xF728,
		KeyLeft = 0xF725,
		KeyRight = 0xF727,

		KeyDelete = 0xF72E,
		KeyBegin = 0xF72D,
		KeyEnd = 0xF72E,

		KeyPageUp = 0xF721,
		KeyPageDown,

		KeyPrintScreen = 0xF72E,
		KeyESC = 0xF71B
#endif

#if RN_PLATFORM_LINUX
		//TODO: Direct copy from windows, probably all wrong...
		KeyReturn = 0xD,

		KeyF1 = 0xF770,
		KeyF2,
		KeyF3,
		KeyF4,
		KeyF5,
		KeyF6,
		KeyF7,
		KeyF8,
		KeyF9,
		KeyF10,
		KeyF11,
		KeyF12,

		KeyUp = 0xF726,
		KeyDown = 0xF728,
		KeyLeft = 0xF725,
		KeyRight = 0xF727,

		KeyDelete = 0xF72E,
		KeyBegin = 0xF72D,
		KeyEnd = 0xF72E,

		KeyPageUp = 0xF721,
		KeyPageDown,

		KeyPrintScreen = 0xF72E,
		KeyESC = 0xF71B
#endif
	};
	
	enum KeyModifier
	{
		KeyShift = (1 << 0),
		KeyAlt = (1 << 1),
		KeyControl = (1 << 2),
		KeyCommand = (1 << 3),
		KeyWindows = KeyCommand,
		KeyCapsLock = (1 << 4),
		
#if RN_PLATFORM_MAC_OS
		KeyAction = KeyCommand
#else
		KeyAction = KeyControl
#endif
	};
	
	class Input;
	
	class Event : public Message
	{
	public:
		friend class Input;
		
		enum class Type
		{
			MouseDown,
			MouseUp,
			MouseMoved,
			MouseDragged,
			MouseWheel,
			KeyDown,
			KeyUp,
			KeyRepeat,
			KeyHeld
		};
		
		Type GetType() const { return _type; }
		
		char GetCharacter() const { return CodePoint::ConverToCharacter(_key); }
		UniChar GetCode() const { return _key; }
		
		const Vector2 &GetMouseDelta() const { return _mouseDelta; }
		const Vector2 &GetMousePosition() const { return _mousePosition; }
		const Vector2 &GetMouseWheel() const { return _mouseWheel; }
		
		bool IsShift() const { return (_modifierKeys & KeyModifier::KeyShift); }
		bool IsControl() const { return (_modifierKeys & KeyModifier::KeyControl); }
		bool IsAlt() const { return (_modifierKeys & KeyModifier::KeyAlt); }
		bool IsCommand() const { return (_modifierKeys & KeyModifier::KeyCommand); }
		
		uint32 GetButton() const { return _button; }
		
		RNAPI bool IsKeyboard() const;
		RNAPI bool IsMouse() const;
		
	private:
		Event();
		
		Type _type;
		
		UniChar _key;
		uint32 _modifierKeys;
		
		uint32 _button;
		
		Vector2 _mouseDelta;
		Vector2 _mousePosition;
		Vector2 _mouseWheel;
		
		RNDeclareMeta(Event)
	};
	
	class Input : public ISingleton<Input>
	{
	public:
		friend class Window;
		friend class Kernel;
		
		RNAPI Input();
		RNAPI ~Input();

		RNAPI void Activate();
		RNAPI void Deactivate();
		
		RNAPI void DispatchInputEvents();
		RNAPI void InvalidateFrame();
		RNAPI void InvalidateMouse();
		
		RNAPI void RegisterDevice(InputDevice *device);
		RNAPI void UnregisterDevice(InputDevice *device);

#if RN_PLATFORM_MAC_OS
		RNAPI void HandleEvent(void *data);
#endif

#if RN_PLATFORM_WINDOWS
		RNAPI void HandleSystemEvent(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
#endif
		
		const Vector2 &GetMouseDelta() const { return _mouseDelta; }
		const Vector2 &GetMousePosition() const { return _mousePosition; }
		const Vector2 &GetMouseWheel() const { return _mouseWheel; }
		
		uint32 GetModifierKeys() const { return _modifierKeys; }
		bool IsActive() const { return _active; }
		
		RNAPI bool IsKeyPressed(char key) const;
		RNAPI bool IsKeyPressed(UniChar key) const;
		
		RNAPI bool IsMousePressed(uint32 button) const;
		
		RNAPI Array *GetInputDevices();
		RNAPI Array *GetInputDevicesMatching(InputDevice::Category categories);

	private:
		void BuildDeviceTree();
		
		void FlushEventQueue();
		Vector2 ClampMousePosition(const Vector2 &position) const;

#if RN_PLATFORM_WINDOWS
		void HandleMouseEvent(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
#endif
		
		SpinLock _deviceLock;
		Array *_inputDevices;
		
		
		Vector2 _mousePosition;
		Vector2 _realMousePosition;
		Vector2 _mouseDelta;
		Vector2 _mouseWheel;
		
		
		uint32 _modifierKeys;
		
		std::vector<Event *> _pendingEvents;
		std::unordered_set<UniChar> _pressedKeys;
		std::unordered_set<uint32> _pressedMouse;

#if RN_PLATFORM_WINDOWS
		bool _windowCaptured;
#endif
		
		SpinLock _lock;
		bool _active;
		bool _invalidateMouse;
		
		RNDeclareSingleton(Input)
	};
}

#endif /* __RAYNE_INPUT_H__ */
