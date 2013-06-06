//
//  RNUIControl.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIControl.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(Control)
		
		Control::Control()
		{
			_state = Control::Normal;
			_mouseDown = false;
		}
		Control::~Control()
		{
		}
		
		
		void Control::SetHighlighted(bool highlighted)
		{
			State oldState = _state;
			_state &= ~Control::Highlighted;
			
			if(highlighted)
				_state |= Control::Highlighted;
			
			if(_state != oldState)
				StateChanged(_state);
		}
		void Control::SetSelected(bool selected)
		{
			State oldState = _state;
			_state &= ~Control::Selected;
			
			if(selected)
				_state |= Control::Selected;
			
			if(_state != oldState)
				StateChanged(_state);
		}
		void Control::SetEnabled(bool enabled)
		{
			State oldState = _state;
			_state &= ~Control::Disabled;
			
			if(!enabled)
				_state |= Control::Disabled;
			
			if(_state != oldState)
				StateChanged(_state);
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
			
			RemoveListener(EventType::ValueChanged, cookie);
		}
		
		void Control::PostEvent(EventType event)
		{
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
			Vector2 point = std::move(ConvertPointFromView(nullptr, event->MousePosition()));
			return Bounds().ContainsPoint(point);
		}
		
		
		void Control::ConsumeMouseClicks(Event *event)
		{
			if(event->EventType() == Event::Type::MouseUp)
			{
				_mouseDown = false;
				
				EventType type = IsEventWithinBounds(event) ? EventType::MouseUpInside : EventType::MouseUpOutside;
				
				SetSelected(false);
				PostEvent(type);
				return;
			}
			
			if(event->EventType() == Event::Type::MouseDown && IsEventWithinBounds(event))
			{
				if(!_mouseDown)
				{
					_mouseDown = true;
					
					SetSelected(true);
					PostEvent(EventType::MouseDown);
				}
				else
				{
					PostEvent(EventType::MouseDownRepeat);
				}
			}
		}
		
		void Control::ConsumeMouseMove(Event *event)
		{
			if(event->EventType() == Event::Type::MouseMoved)
			{
				bool isWithinBounds = IsEventWithinBounds(event);
				SetHighlighted(isWithinBounds);
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
			
			_mouseDown = false;
			
			_state &= ~Control::Highlighted;
			_state &= ~Control::Selected;
			
			StateChanged(_state);
		}
	}
}
