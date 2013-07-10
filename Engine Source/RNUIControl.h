//
//  RNUIControl.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UICONTROL_H__
#define __RAYNE_UICONTROL_H__

#include "RNBase.h"
#include "RNInput.h"
#include "RNUIView.h"

namespace RN
{
	namespace UI
	{
		class Control : public View
		{
		public:
			enum
			{
				Normal = (1 << 0),
				Highlighted = (1 << 1),
				Selected = (1 << 2),
				Disabled = (1 << 3)
			};
			typedef uint32 State;
			
			enum class EventType
			{
				MouseDown,
				MouseDownRepeat,
				MouseUpInside,
				MouseUpOutside,
				
				ValueChanged
			};
			
			typedef std::function<void (Control *, EventType)> Callback;
			
			void SetHighlighted(bool highlighted);
			void SetSelected(bool selected);
			void SetEnabled(bool enabled);
			
			bool IsHighlighted() const { return _state & Control::Highlighted; }
			bool IsSelected() const { return _state & Control::Selected; }
			bool IsEnabled() const { return !(_state & Control::Disabled); }
			
			State ControlState() const { return _state; }
			
			void AddListener(EventType event, Callback callback, void *cookie);
			void RemoveListener(EventType event, void *cookie);
			void RemoveListener(void *cookie);
			
			virtual void BeginTrackingEvent(Event *event);
			virtual void ContinueTrackingEvent(Event *event);
			virtual void EndTrackingEvent(Event *event);
		
		protected:
			Control();
			~Control() override;
			
			virtual void StateChanged(State state);
			
			void PostEvent(EventType event);
			bool IsEventWithinBounds(Event *event);
			
		private:
			void ConsumeMouseClicks(Event *event);
			void ConsumeMouseMove(Event *event);
			
			State _state;
			
			bool _mouseDown;
			
			struct EventListener
			{
				Callback callback;
				void *cookie;
			};
			
			std::map<EventType, std::vector<EventListener>> _listener;
			
			RNDefineMeta(Control, View)
		};
	}
}

#endif /* __RAYNE_UICONTROL_H__ */
