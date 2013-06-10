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
		
		KeyDelete = 0xF727,
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
		
		Type EventType() const { return _type; }
		
		char Character() const { return CodePoint::ConverToCharacter(_key); }
		UniChar Code() const { return _key; }
		
		const Vector2& MouseDelta() const { return _mouseDelta; }
		const Vector2& MousePosition() const { return _mousePosition; }
		const Vector2& MouseWheel() const { return _mousePosition; }
		
		bool Shift() const { return (_modifierKeys & KeyModifier::KeyShift); }
		bool Control() const { return (_modifierKeys & KeyModifier::KeyControl); }
		bool Alt() const { return (_modifierKeys & KeyModifier::KeyAlt); }
		bool Command() const { return (_modifierKeys & KeyModifier::KeyCommand); }
		
		uint32 Button() const { return _button; }
		
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
		RNAPI void HandleEvent(void *data);
		
		const Vector2& MouseDelta() const { return _mouseDelta; }
		const Vector2& MousePosition() const { return _mousePosition; }
		const Vector2& MouseWheel() const { return _mouseWheel; }
		
		uint32 ModifierKeys() const { return _modifierKeys; }
		
		RNAPI bool KeyPressed(char key) const;
		RNAPI bool KeyPressed(UniChar key) const;

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
		
		SpinLock _lock;
		bool _active;
	};
}

#endif /* __RAYNE_INPUT_H__ */
