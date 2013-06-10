//
//  RNInput.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBaseInternal.h"
#include "RNInput.h"
#include "RNAutoreleasePool.h"
#include "RNWindow.h"

namespace RN
{
	RNDeclareMeta(Event)
	
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
	
	
	
	void TranslateKeyboardEvent(NSEvent *event, const std::function<void (UniChar)>& callback)
	{
		NSString *characters = [event charactersIgnoringModifiers];
		for(NSUInteger i=0; i<[characters length]; i++)
		{
			UniChar character = static_cast<UniChar>([characters characterAtIndex:i]);
			CodePoint codePoint(character);
			
			callback(codePoint.LowerCase());
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
	
	
	
	
	Input::Input()
	{
		_active = true;
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
		_lock.Unlock();
	}
	
	void Input::Deactivate()
	{
		_lock.Lock();
		
		_active = false;
		FlushEventQueue();
		
		_lock.Unlock();
	}
	
	
	
	bool Input::KeyPressed(char key) const
	{
		return KeyPressed(CodePoint::ConvertCharacter(key));
	}
	
	bool Input::KeyPressed(UniChar key) const
	{
		return (_pressedKeys.find(key) != _pressedKeys.end());
	}
	
	
	Vector2 Input::ClampMousePosition(const Vector2& position) const
	{
		Rect frame = Window::SharedInstance()->Frame();
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
		
		MessageCenter *messageCenter = MessageCenter::SharedInstance();
		
		std::vector<Event *> events;
		std::swap(events, _pendingEvents);
		
		_lock.Unlock();
		
		_modifierKeys = TranslateModifierFlags([NSEvent modifierFlags]);
		
		for(Event *event : events)
		{
			if(_active)
				messageCenter->PostMessage(event);
			
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
	
	void Input::HandleEvent(void *data)
	{
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
							_pressedKeys.insert(character);
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
							
							_pressedKeys.erase(character);
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
				
				break;
			}
			
			case NSRightMouseDragged:
			case NSLeftMouseDragged:
			case NSOtherMouseDragged:
			{
				Vector2 position = std::move(TranslateMouseEvent(nsevent));
				Vector2 delta = _realMousePosition - position;
				
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
				Vector2 delta = _realMousePosition - position;
				
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
				break;
			}
				
			default:
				break;
		}
		
		if(event)
			_pendingEvents.push_back(event);
		
		delete pool;
	}
}
