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
#include "RNWrappingObject.h"
#include "RNSpinLock.h"
#include "RNVector.h"

namespace RN
{
	
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
	};
	
	enum KeyModifier
	{
		KeyShift = (1 << 0),
		KeyAlt = (1 << 1),
		KeyControl = (1 << 2),
		KeyCommand = (1 << 3),
		KeyWindows = KeyCommand,
		KeyCapsLock = (1 << 4)
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
		
		const Vector2& GetMouseDelta() const { return _mouseDelta; }
		const Vector2& GetMousePosition() const { return _mousePosition; }
		const Vector2& GetMouseWheel() const { return _mouseWheel; }
		
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
	friend class Window;
	friend class Kernel;
	public:
		RNAPI Input();
		RNAPI ~Input();

		RNAPI void Activate();
		RNAPI void Deactivate();
		
		RNAPI void DispatchInputEvents();
		RNAPI void InvalidateFrame();
		RNAPI void InvalidateMouse();

#if RN_PLATFORM_MAC_OS
		RNAPI void HandleEvent(void *data);
#endif

#if RN_PLATFORM_WINDOWS
		RNAPI void HandleSystemEvent(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
#endif
		
		const Vector2& GetMouseDelta() const { return _mouseDelta; }
		const Vector2& GetMousePosition() const { return _mousePosition; }
		const Vector2& GetMouseWheel() const { return _mouseWheel; }
		
		uint32 GetModifierKeys() const { return _modifierKeys; }
		bool IsActive() const { return _active; }
		
		RNAPI bool IsKeyPressed(char key) const;
		RNAPI bool IsKeyPressed(UniChar key) const;
		
		RNAPI bool IsMousePressed(uint32 button) const;

	private:
		void FlushEventQueue();
		Vector2 ClampMousePosition(const Vector2& position) const;

#if RN_PLATFORM_WINDOWS
		void HandleMouseEvent(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
#endif
		
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
