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
				Normal = 0,
				Highlighted = (1 << 0),
				Selected = (1 << 1),
				Disabled = (1 << 2)
			};
			typedef uint32 State;
			
			enum class EventType
			{
				MouseDown,
				MouseDownRepeat,
				MouseUpInside,
				MouseUpOutside,
				
				MouseEntered,
				MouseLeft,
				
				ValueChanged
			};
			
			typedef std::function<void (Control *, EventType)> Callback;
			
			virtual void SetHighlighted(bool highlighted);
			virtual void SetSelected(bool selected);
			virtual void SetEnabled(bool enabled);
			
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
			
			void MouseDown(Event *event) override;
			void MouseMoved(Event *event) override;
			void MouseUp(Event *event) override;
		
		protected:
			Control();
			~Control() override;
			
			void SetState(State state);
			bool IsEventWithinBounds(Event *event);
			
			virtual void StateChanged(State state);
			virtual bool PostEvent(EventType event);
			void DispatchEvent(EventType event);
			
		private:			
			void ConsumeMouseClicks(Event *event);
			void ConsumeMouseMove(Event *event);
			
			State _state;
			
			bool _mouseDown;
			bool _mouseInside;
			
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
