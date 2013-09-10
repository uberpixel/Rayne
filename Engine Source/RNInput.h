//
//  RNInput.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		
		KeyPrintScreen = 0xF72E
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
	friend class Input;
	public:
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
		
		bool IsKeyboard() const;
		bool IsMouse() const;
		
	private:
		Event();
		
		Type _type;
		
		UniChar _key;
		uint32 _modifierKeys;
		
		uint32 _button;
		
		Vector2 _mouseDelta;
		Vector2 _mousePosition;
		Vector2 _mouseWheel;
		
		RNDefineMeta(Event, Message)
	};
	
	class Input : public Singleton<Input>
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
		RNAPI void HandleEvent(void *data);
		
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
		
		Vector2 _mousePosition;
		Vector2 _realMousePosition;
		Vector2 _mouseDelta;
		Vector2 _mouseWheel;
		
		
		uint32 _modifierKeys;
		
		std::vector<Event *> _pendingEvents;
		std::unordered_set<UniChar> _pressedKeys;
		std::unordered_set<uint32> _pressedMouse;
		
		SpinLock _lock;
		bool _active;
		bool _invalidateMouse;
	};
}

#endif /* __RAYNE_INPUT_H__ */
