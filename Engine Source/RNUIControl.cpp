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
		Control::Control()
		{
			_state = Control::Normal;
		}
		Control::~Control()
		{
		}
		
		
		void Control::SetHighlighted(bool highlighted)
		{
			_state &= ~Control::Highlighted;
			
			if(highlighted)
				_state |= Control::Highlighted;
			
			StateChanged(_state);
		}
		void Control::SetSelected(bool selected)
		{
			_state &= ~Control::Selected;
			
			if(selected)
				_state |= Control::Selected;
			
			StateChanged(_state);
		}
		void Control::SetEnabled(bool enabled)
		{
			_state &= ~Control::Disabled;
			
			if(!enabled)
				_state |= Control::Disabled;
			
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
			RemoveListener(EventType::StateChanged, cookie);
		}
		
		void Control::PostEvent(EventType event)
		{
			auto iterator = _listener.find(event);
			if(iterator == _listener.end())
				return;
			
			std::vector<EventListener>& listener = iterator->second;
			for(auto i=listener.begin(); i!=listener.end();)
			{
				i->callback(this, event);
			}
		}
		
		
		Vector2 Control::LocationOfEvent(Event *event)
		{
			Vector2 position = event->MousePosition();
			return std::move(ConvertPointToView(this, position));
		}
		bool Control::EventIsInsideFrame(Event *event)
		{
			Rect rect = std::move(ConvertRectToView(nullptr, Frame()));
			return rect.ContainsPoint(event->MousePosition());
		}
		
		
		void Control::BeginTrackingEvent(Event *event)
		{
			if(event->IsMouse())
			{
				
			}
		}
		
		void Control::ContinueTrackingEvent(Event *event)
		{
		}
		
		void Control::EndTrackingEvent(Event *event)
		{
		}
	}
}
