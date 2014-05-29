//
//  RNUIControl.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIControl.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Control, View)
		
		Control::Control()
		{
			_state = Control::State::Normal;
			
			_mouseDown = false;
			_mouseInside = false;
		}
		Control::Control(const Rect &frame) :
			View(frame)
		{
			_state = Control::State::Normal;
			
			_mouseDown = false;
			_mouseInside = false;
		}
		Control::~Control()
		{
		}
		
		
		void Control::SetHighlighted(bool highlighted)
		{
			State oldState = _state;
			_state &= ~Control::State::Highlighted;
			
			if(highlighted)
				_state |= Control::State::Highlighted;
			
			if(_state != oldState)
				StateChanged(_state);
		}
		void Control::SetSelected(bool selected)
		{
			State oldState = _state;
			_state &= ~Control::State::Selected;
			
			if(selected)
				_state |= Control::State::Selected;
			
			if(_state != oldState)
				StateChanged(_state);
		}
		void Control::SetEnabled(bool enabled)
		{
			State oldState = _state;
			_state &= ~Control::State::Disabled;
			
			if(!enabled)
			{
				_state |= Control::State::Disabled;
				_mouseDown = false;
			}
			
			if(_state != oldState)
				StateChanged(_state);
		}
		
		void Control::SetState(State state)
		{
			_state = state;
			
			if(!IsEnabled())
				_mouseDown = false;
		}
		
		void Control::StateChanged(State state)
		{}
		
		void Control::AddListener(EventType event, Callback callback, void *cookie)
		{
			EventListener listener;
			listener.cookie = cookie;
			listener.callback = std::move(callback);
			
			auto i = _listener.find(event);
			if(i == _listener.end())
			{
				std::vector<EventListener> temp;
				temp.push_back(std::move(listener));
				
				_listener.insert(std::map<EventType, std::vector<EventListener>>::value_type(event, std::move(temp)));
			}
			else
			{
				i->second.push_back(std::move(listener));
			}
		}
		void Control::RemoveListener(EventType event, void *cookie)
		{
			auto iterator = _listener.find(event);
			if(iterator == _listener.end())
				return;
			
			std::vector<EventListener>& listener = iterator->second;
			for(auto i=listener.begin(); i!=listener.end();)
			{
				if(i->cookie == cookie)
				{
					i = listener.erase(i);
					continue;
				}
				
				i ++;
			}
			
			if(listener.size() == 0)
				_listener.erase(event);
		}
		void Control::RemoveListener(void *cookie)
		{
			RemoveListener(EventType::MouseDown, cookie);
			RemoveListener(EventType::MouseDownRepeat, cookie);
			RemoveListener(EventType::MouseUpInside, cookie);
			RemoveListener(EventType::MouseUpOutside, cookie);
			
			RemoveListener(EventType::MouseEntered, cookie);
			RemoveListener(EventType::MouseLeft, cookie);
			
			RemoveListener(EventType::ValueChanged, cookie);
		}
		
		bool Control::PostEvent(EventType event)
		{
			if(_state & Control::State::Disabled)
				return false;
			
			switch(event)
			{
				case EventType::MouseDown:
					SetSelected(true);
					break;
					
				case EventType::MouseUpInside:
				case EventType::MouseUpOutside:
					SetSelected(false);
					break;
					
				case EventType::MouseEntered:
					SetHighlighted(true);
					break;
					
				case EventType::MouseLeft:
					SetHighlighted(false);
					break;
					
				default:
					break;
			}
			
			return true;
		}
		
		void Control::DispatchEvent(EventType event)
		{
			if(!PostEvent(event))
				return;
			
			auto iterator = _listener.find(event);
			if(iterator == _listener.end())
				return;
			
			std::vector<EventListener>& listener = iterator->second;
			for(auto i=listener.begin(); i!=listener.end(); i++)
			{
				i->callback(this, event);
			}
		}
		
		
		bool Control::IsEventWithinBounds(Event *event)
		{
			return GetBounds().ContainsPoint(GetLocationForEvent(event));
		}
		
		Vector2 Control::GetLocationForEvent(Event *event)
		{
			Vector2 point = event->GetMousePosition();
			point = std::move(ConvertPointFromBase(point));
			
			return point;
		}
	
		
		
		void Control::ConsumeMouseClicks(Event *event)
		{
			if(event->GetType() == Event::Type::MouseUp && _mouseDown)
			{
				_mouseDown = false;
				
				EventType type = IsEventWithinBounds(event) ? EventType::MouseUpInside : EventType::MouseUpOutside;
				DispatchEvent(type);
				return;
			}
			
			if(event->GetType() == Event::Type::MouseDown && IsEventWithinBounds(event))
			{
				if(!_mouseDown)
				{
					_mouseDown = true;
					DispatchEvent(EventType::MouseDown);
				}
				else
				{
					DispatchEvent(EventType::MouseDownRepeat);
				}
			}
		}
		
		void Control::ConsumeMouseMove(Event *event)
		{
			if(event->GetType() == Event::Type::MouseMoved)
			{
				bool isWithinBounds = IsEventWithinBounds(event);
				
				if(!_mouseInside && isWithinBounds)
				{
					DispatchEvent(EventType::MouseEntered);
					_mouseInside = true;
				}
				
				if(_mouseInside && !isWithinBounds)
				{
					DispatchEvent(EventType::MouseLeft);
					_mouseInside = false;
				}
			}
		}
		
		
		
		void Control::BeginTrackingEvent(Event *event)
		{
			if(event->IsMouse())
			{
				ConsumeMouseClicks(event);
				ConsumeMouseMove(event);
			}
		}
		
		void Control::ContinueTrackingEvent(Event *event)
		{
			if(event->IsMouse())
			{
				ConsumeMouseClicks(event);
				ConsumeMouseMove(event);
			}
		}
		
		void Control::EndTrackingEvent(Event *event)
		{
			if(event->IsMouse())
			{
				ConsumeMouseClicks(event);
				ConsumeMouseMove(event);
			}
		}
		
		
		void Control::MouseDown(Event *event)
		{
			GetWidget()->MakeFirstResponder(this);
			BeginTrackingEvent(event);
		}
		void Control::MouseMoved(Event *event)
		{
			ContinueTrackingEvent(event);
		}
		void Control::MouseUp(Event *event)
		{
			EndTrackingEvent(event);
		}
		void Control::MouseLeft(Event *event)
		{
			if(_mouseInside)
			{
				DispatchEvent(EventType::MouseLeft);
				_mouseInside = false;
			}
		}
	}
}
