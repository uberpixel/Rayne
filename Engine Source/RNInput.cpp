//
//  RNInput.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBaseInternal.h"
#include "RNInput.h"
#include "RNAutoreleasePool.h"
#include "RNWindow.h"
#include "RNUIServer.h"

namespace RN
{
	RNDeclareMeta(Event)
	RNDeclareSingleton(Input)
	
	Event::Event() :
		Message(kRNInputEventMessage, nullptr, nullptr)
	{}
	
	bool Event::IsKeyboard() const
	{
		switch(_type)
		{
			case Type::KeyDown:
			case Type::KeyUp:
			case Type::KeyRepeat:
			case Type::KeyHeld:
				return true;
				
			default:
				return false;
		}
	}
	
	bool Event::IsMouse() const
	{
		switch(_type)
		{
			case Type::MouseDown:
			case Type::MouseUp:
			case Type::MouseMoved:
			case Type::MouseDragged:
			case Type::MouseWheel:
				return true;
				
			default:
				return false;
		}
	}
	
	
#if RN_PLATFORM_MAC_OS
	void TranslateKeyboardEvent(NSEvent *event, const std::function<void (UniChar)>& callback)
	{
		NSString *characters = [event characters];
		for(NSUInteger i=0; i<[characters length]; i++)
		{
			UniChar character = static_cast<UniChar>([characters characterAtIndex:i]);
			callback(character);
		}
	}
	
	uint32 TranslateModifierFlags(NSUInteger nsflags)
	{
		uint32 flags = 0;
		
		flags |= (nsflags & NSShiftKeyMask) ? KeyModifier::KeyShift : 0;
		flags |= (nsflags & NSAlternateKeyMask) ? KeyModifier::KeyAlt : 0;
		flags |= (nsflags & NSControlKeyMask) ? KeyModifier::KeyControl : 0;
		flags |= (nsflags & NSCommandKeyMask) ? KeyModifier::KeyCommand : 0;
		flags |= (nsflags & NSAlphaShiftKeyMask) ? KeyModifier::KeyCapsLock : 0;
		
		return flags;
	}
	
	Vector2 TranslateMouseEvent(NSEvent *event)
	{
		NSWindow *window = [event window];
		NSPoint point = [event locationInWindow];
		NSSize  size = [[window contentView] bounds].size;
		
		point.y = size.height - point.y;
		
		Vector2 result;
		
		result.x = roundf(static_cast<float>(point.x));
		result.y = roundf(static_cast<float>(point.y));
		
		return result;
	}
	
	Vector2 TranslateMouseDelta(NSEvent *event, bool& ignoreDelta)
	{
		if(ignoreDelta)
		{
			ignoreDelta = false;
			return Vector2();
		}
		
		Vector2 delta;
		
		delta.x = -[event deltaX];
		delta.y = -[event deltaY];
		
		return delta;
	}
#endif
#if RN_PLATFORM_WINDOWS
	uint32 TranslateModifierFlags(BYTE *keyState)
	{
		uint32 flags = 0;

		flags |= (keyState[VK_SHIFT]) ? KeyModifier::KeyShift : 0;
		flags |= (keyState[LVKF_ALT]) ? KeyModifier::KeyAlt : 0;
		flags |= (keyState[VK_CONTROL]) ? KeyModifier::KeyControl : 0;
		flags |= (keyState[VK_LWIN] || keyState[VK_RWIN]) ? KeyModifier::KeyCommand : 0;
		flags |= (keyState[VK_CAPITAL]) ? KeyModifier::KeyCapsLock : 0;

		return flags;
	}

	bool CheckVKey(int vcode)
	{
		return (GetAsyncKeyState(vcode) & 0x8000);
	}

	uint32 TranslateModifierFlags()
	{
		uint32 flags = 0;

		flags |= (CheckVKey(VK_SHIFT)) ? KeyModifier::KeyShift : 0;
		flags |= (CheckVKey(LVKF_ALT)) ? KeyModifier::KeyAlt : 0;
		flags |= (CheckVKey(VK_CONTROL)) ? KeyModifier::KeyControl : 0;
		flags |= (CheckVKey(VK_LWIN) || CheckVKey(VK_RWIN)) ? KeyModifier::KeyCommand : 0;
		flags |= (CheckVKey(VK_CAPITAL)) ? KeyModifier::KeyCapsLock : 0;

		return flags;
	}
#endif
	
	
	Input::Input()
	{
		_active = true;
		_invalidateMouse = true;
	}
	
	Input::~Input()
	{
		for(Event *event : _pendingEvents)
		{
			event->Release();
		}
	}
	
	
	void Input::Activate()
	{
		_lock.Lock();
		_active = true;
		_invalidateMouse = true;
		_lock.Unlock();
	}
	
	void Input::Deactivate()
	{
		_lock.Lock();
		
		_active = false;
		FlushEventQueue();
		
		
		_pressedKeys.clear();
		_modifierKeys = 0;
		_lock.Unlock();
	}
	
	
	
	bool Input::IsKeyPressed(char key) const
	{
		return IsKeyPressed(CodePoint::ConvertCharacter(key));
	}
	
	bool Input::IsKeyPressed(UniChar key) const
	{
		return (_pressedKeys.find(key) != _pressedKeys.end());
	}
	
	bool Input::IsMousePressed(uint32 button) const
	{
		return (_pressedMouse.find(button) != _pressedMouse.end());
	}
	
	
	Vector2 Input::ClampMousePosition(const Vector2& position) const
	{
		Rect frame = Window::GetSharedInstance()->GetFrame();
		Vector2 result;
		
		result.x = roundf(std::min(frame.width, std::max(0.0f, position.x)));
		result.y = roundf(std::min(frame.height, std::max(0.0f, position.y)));
		
		return result;
	}
	
	
	void Input::FlushEventQueue()
	{
		for(Event *event : _pendingEvents)
		{
			event->Release();
		}
		
		_pendingEvents.clear();
	}
	
	void Input::DispatchInputEvents()
	{
		_lock.Lock();
		
		if(!_active)
		{
			_lock.Unlock();
			return;
		}
		
		MessageCenter *messageCenter = MessageCenter::GetSharedInstance();
		UI::Server *server = UI::Server::GetSharedInstance();
		
		std::vector<Event *> events;
		std::swap(events, _pendingEvents);
		
		_lock.Unlock();
		
#if RN_PLATFORM_MAC_OS
		_modifierKeys = TranslateModifierFlags([NSEvent modifierFlags]);
#endif

#if RN_PLATFORM_WINDOWS
		_modifierKeys = TranslateModifierFlags();
#endif
		
		for(Event *event : events)
		{
			if(_active)
			{
				if(server->ConsumeEvent(event))
				{
					if(event->IsKeyboard())
					{
						UniChar code = event->GetCode();
						_pressedKeys.erase(CodePoint(code).GetLowerCase());
					}
					
					event->Release();
					continue;
				}
				
				messageCenter->PostMessage(event);
			}
			
			if(event->_type == Event::Type::KeyDown)
			{
				event->_type = Event::Type::KeyHeld;
				_pendingEvents.push_back(event);
			}
			else
			{
				event->Release();
			}
		}
	}
	
	void Input::InvalidateFrame()
	{
		_mouseDelta = Vector2(0.0f);
		_mouseWheel = Vector2(0.0f);
	}
	
	void Input::InvalidateMouse()
	{
		_invalidateMouse = true;
	}
	
#if RN_PLATFORM_MAC_OS
	void Input::HandleEvent(void *data)
	{
		if(!_active)
			return;
	
		AutoreleasePool *pool = new AutoreleasePool();
		
		NSEvent *nsevent = static_cast<NSEvent *>(data);
		Event *event = new Event();
		
		switch([nsevent type])
		{
			case NSKeyDown:
			case NSKeyUp:
			{
				Event::Type type;
				_modifierKeys = TranslateModifierFlags([nsevent modifierFlags]);
				
				switch([nsevent type])
				{
					case NSKeyDown:
						type = ([nsevent isARepeat]) ? Event::Type::KeyRepeat : Event::Type::KeyDown;
						break;
						
					case NSKeyUp:
						type = Event::Type::KeyUp;
						break;
				}
				
				event->Release();
				event = nullptr;
				
				TranslateKeyboardEvent(nsevent, [&](UniChar character) {
					
					event = new Event();
					event->_type = type;
					event->_modifierKeys = _modifierKeys;
					event->_key = character;
					
					switch(type)
					{
						case Event::Type::KeyDown:
							_pressedKeys.insert(CodePoint(character).GetLowerCase());
							break;
							
						case Event::Type::KeyUp:
						{
							for(auto i=_pendingEvents.begin(); i!=_pendingEvents.end();)
							{
								Event *temp = *i;
								if(temp->_type == Event::Type::KeyHeld && temp->_key == character)
								{
									temp->Release();
									
									i = _pendingEvents.erase(i);
									continue;
								}
								
								i++;
							}
							
							_pressedKeys.erase(CodePoint(character).GetLowerCase());
							break;
						}
							
						default:
							break;
					}
					
					_pendingEvents.push_back(event);
				});
				
				event = nullptr;
				break;
			}
				
			case NSLeftMouseDown:
			case NSRightMouseDown:
			case NSOtherMouseDown:
			{
				event->_type = Event::Type::MouseDown;
				event->_button = static_cast<uint32>([nsevent buttonNumber]);
				event->_mouseDelta = _mouseDelta;
				event->_mousePosition = _mousePosition;
				_pressedMouse.insert(event->_button);
				break;
			}
				
			case NSLeftMouseUp:
			case NSRightMouseUp:
			case NSOtherMouseUp:
			{
				event->_type = Event::Type::MouseUp;
				event->_button = static_cast<uint32>([nsevent buttonNumber]);
				event->_mouseDelta = _mouseDelta;
				event->_mousePosition = _mousePosition;
				_pressedMouse.erase(event->_button);
				break;
			}
			
			case NSRightMouseDragged:
			case NSLeftMouseDragged:
			case NSOtherMouseDragged:
			{
				Vector2 position = std::move(TranslateMouseEvent(nsevent));
				Vector2 delta    = std::move(TranslateMouseDelta(nsevent, _invalidateMouse));
				
				_mouseDelta += delta;
				_realMousePosition = std::move(position);
				_mousePosition = std::move(ClampMousePosition(_realMousePosition));
				
				event->_button = static_cast<uint32>([nsevent buttonNumber]);
				event->_type = Event::Type::MouseDragged;
				event->_mouseDelta = delta;
				event->_mousePosition = _mousePosition;
				break;
			}
				
			case NSMouseMoved:
			{
				Vector2 position = std::move(TranslateMouseEvent(nsevent));
				Vector2 delta    = std::move(TranslateMouseDelta(nsevent, _invalidateMouse));
				
				_mouseDelta += delta;
				_realMousePosition = std::move(position);
				_mousePosition = std::move(ClampMousePosition(_realMousePosition));
				
				event->_type = Event::Type::MouseMoved;
				event->_mouseDelta = delta;
				event->_mousePosition = _mousePosition;
				
				break;
			}
				
			case NSScrollWheel:
			{
				_mouseWheel += Vector2([nsevent deltaX], [nsevent deltaY]);
				
				event->_type = Event::Type::MouseWheel;
				event->_mouseWheel = _mouseWheel;
				event->_mousePosition = _mousePosition;
				break;
			}
				
			default:
				break;
		}
		
		if(event)
			_pendingEvents.push_back(event);
		
		delete pool;
	}
#endif

#if RN_PLATFORM_WINDOWS
	void Input::HandleSystemEvent(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
	{
		switch(message)
		{
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MOUSEWHEEL:
			case WM_MOUSEMOVE:
			{
				HandleMouseEvent(window, message, wparam, lparam);
				break;
			}

			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				Event *event = new Event();

				BYTE keyState[256];
				::GetKeyboardState(keyState);

				uint32 scan = (lparam >> 16) & 0xff;
				uint32 code = (wparam < 0x0030) ? 1 : 0;

				WCHAR buffer = 0;
				if(::ToUnicode((UINT)wparam, scan, keyState, &buffer, 1, 0) == 1)
					code = buffer;

				event->_key  = (UniChar)code;
				event->_type = (message == WM_KEYDOWN) ? Event::Type::KeyDown : Event::Type::KeyUp;
				event->_modifierKeys = TranslateModifierFlags(keyState);

				if(message == WM_KEYDOWN)
				{
					uint32 repeatCount = (lparam & 0xf);
					if(repeatCount > 0)
						event->_type = Event::Type::KeyRepeat;

					_pressedKeys.insert(CodePoint((UniChar)code).GetLowerCase());
				}
				else
				{
					_pressedKeys.erase(CodePoint((UniChar)code).GetLowerCase());
				}

				_pendingEvents.push_back(event);
				break;
			}
		}
	}

	void Input::HandleMouseEvent(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
	{
		Event *event = new Event();

		Vector2 position = Vector2((float)(int16)LOWORD(lparam), (float)(int16)HIWORD(lparam));
		Vector2 delta = _mousePosition - position;

		_mouseDelta += delta;
		_realMousePosition = std::move(position);
		_mousePosition = std::move(ClampMousePosition(_realMousePosition));
		
		event->_mouseDelta = delta;
		event->_mousePosition = _mousePosition;

		switch(message)
		{
			case WM_LBUTTONDOWN:
				event->_type = Event::Type::MouseDown;
				event->_button = 1;

				_pressedMouse.insert(1);
				break;
			case WM_LBUTTONUP:
				event->_type = Event::Type::MouseUp;
				event->_button = 1;

				_pressedMouse.erase(1);
				break;

			case WM_RBUTTONDOWN:
				event->_type = Event::Type::MouseDown;
				event->_button = 2;

				_pressedMouse.insert(2);
				break;
			case WM_RBUTTONUP:
				event->_type = Event::Type::MouseUp;
				event->_button = 2;

				_pressedMouse.erase(2);
				break;

			case WM_MBUTTONDOWN:
				event->_type = Event::Type::MouseDown;
				event->_button = 3;

				_pressedMouse.insert(3);
				break;
			case WM_MBUTTONUP:
				event->_type = Event::Type::MouseUp;
				event->_button = 3;

				_pressedMouse.erase(3);
				break;

			case WM_MOUSEWHEEL:
			{
				int32 delta = GET_WHEEL_DELTA_WPARAM(wparam);
				int32 k = delta / WHEEL_DELTA;

				if(k != 0)
				{
					event->_type = Event::Type::MouseWheel;
					event->_mouseWheel = Vector2((float)k, 0.0f);
				}

				break;
			}

			case WM_MOUSEMOVE:
				event->_type = (wparam & MK_LBUTTON) ? Event::Type::MouseDragged : Event::Type::MouseMoved;
				break;
		}

		switch(message)
		{
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
				if(_pressedMouse.empty() && _windowCaptured)
				{
					::ReleaseCapture();
					_windowCaptured = false;
				}

				if(!_pressedMouse.empty() && !_windowCaptured)
				{
					::SetCapture(window);
					_windowCaptured = true;
				}
				break;
		}

		_pendingEvents.push_back(event);
		return;
	}
#endif
}
