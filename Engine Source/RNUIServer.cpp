//
//  RNUIServer.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIServer.h"
#include "RNWorld.h"
#include "RNWindow.h"

namespace RN
{
	namespace UI
	{
		Server::Server()
		{
			uint32 flags = Camera::FlagOrthogonal | Camera::FlagUpdateAspect | Camera::FlagUpdateStorageFrame | Camera::FlagNoSorting;
			_camera = new Camera(Vector2(0.0f), TextureParameter::Format::RGBA8888, flags, RenderStorage::BufferFormatColor);
			_camera->SetClearColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
			_camera->SetAllowsDepthWrite(false);
			_camera->SetUseBlending(true);
			
			_camera->clipnear = -500.0f;
			
			if(_camera->Container())
				_camera->Container()->RemoveSceneNode(_camera);
			
			_mainWidget = nullptr;
			_activeControl = nullptr;
			_mode = Mode::SingleTracking;
			
			MessageCenter::SharedInstance()->AddObserver(kRNInputEventMessage, std::bind(&Server::HandleEvent, this, std::placeholders::_1), this);
		}
		
		Server::~Server()
		{
			MessageCenter::SharedInstance()->RemoveObserver(this);
			_camera->Release();
		}
		
		void Server::AddWidget(Widget *widget)
		{
			RN_ASSERT0(widget->_server == 0);
			
			_widgets.push_front(widget);
			widget->_server = this;
			widget->Retain();
		}
		
		void Server::RemoveWidget(Widget *widget)
		{
			RN_ASSERT0(widget->_server == this);
			
			_widgets.erase(std::remove(_widgets.begin(), _widgets.end(), widget), _widgets.end());
			widget->_server = 0;
			widget->Release();
		}
		
		void Server::MoveWidgetToFront(Widget *widget)
		{
			RN_ASSERT0(widget->_server == this);
			
			_widgets.erase(std::remove(_widgets.begin(), _widgets.end(), widget), _widgets.end());
			_widgets.push_front(widget);
		}
		
		
		void Server::NotifyHitControl(Control *control, Event *event)
		{
			if(control == _activeControl)
			{
				control->ContinueTrackingEvent(event);
				return;
			}
			
			if(!_activeControl)
			{
				control->BeginTrackingEvent(event);
				_activeControl = control;
				
				return;
			}
		}
		
		void Server::HandleEvent(Message *message)
		{
			Event *event = static_cast<Event *>(message);
			
			if(event->IsMouse())
			{
				const Vector2& position = event->MousePosition();
				View *hit = nullptr;
				
				for(Widget *widget : _widgets)
				{
					if(widget->Frame().ContainsPoint(position))
					{
						Vector2 transformed = widget->ContentView()->ConvertPointFromView(nullptr, position);
						hit = widget->ContentView()->HitTest(transformed, event);
						
						break;
					}
				}
				
				if(hit && hit->IsKindOfClass(Control::MetaClass()))
				{
					Control *control = static_cast<Control *>(hit);
					NotifyHitControl(control, event);
				}
				else if(_activeControl)
				{
					_activeControl->EndTrackingEvent(event);
					_activeControl = nullptr;
				}
			}
		}
		
		
		void Server::Render(Renderer *renderer)
		{
			Rect actualFrame = Window::SharedInstance()->Frame();
			if(_frame != actualFrame)
			{
				_frame = actualFrame;
					
				_camera->ortholeft   = _frame.Left();
				_camera->orthobottom = _frame.Bottom();
				_camera->orthoright  = _frame.Right();
				_camera->orthotop    = _frame.Top();
				
				_camera->SetFrame(_frame);
				_camera->PostUpdate();
			}
			
			// Draw all widgets into the camera
			renderer->SetMode(Renderer::Mode::ModeUI);
			renderer->BeginCamera(_camera);
			
			for(Widget *widget : _widgets)
			{
				widget->Render(renderer);
			}
			
			renderer->FinishCamera();
		}
	}
}
