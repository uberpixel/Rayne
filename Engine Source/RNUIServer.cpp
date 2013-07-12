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
#include "RNUILabel.h"

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
			
			_debugWidget = nullptr;
			_drawDebugFrames = false;
			
			MessageCenter::SharedInstance()->AddObserver(kRNInputEventMessage, &Server::HandleEvent, this, this);
		}
		
		Server::~Server()
		{
			MessageCenter::SharedInstance()->RemoveObserver(this);
			_camera->Release();
		}
		
		void Server::SetDrawDebugFrames(bool drawDebugFrames)
		{
			_drawDebugFrames = drawDebugFrames;
		}
		
		void Server::AddWidget(Widget *widget)
		{
			RN_ASSERT0(widget->_server == nullptr);
			
			_widgets.push_front(widget);
			widget->_server = this;
			widget->Retain();
		}
		
		void Server::RemoveWidget(Widget *widget)
		{
			RN_ASSERT0(widget->_server == this);
			
			_widgets.erase(std::remove(_widgets.begin(), _widgets.end(), widget), _widgets.end());
			widget->_server = nullptr;
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
			
			if(control != _activeControl)
			{
				_activeControl->EndTrackingEvent(event);
				_activeControl = control;
			
				_activeControl->BeginTrackingEvent(event);
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
		
		
		
		Widget *Server::DebugWidget()
		{
			if(!_debugWidget)
			{
				Label *fpsLabel = new Label();
				fpsLabel->SetText(RNSTR("Praesent commodo cursus magna, vel scelerisque nisl consectetur et. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Duis mollis, est non commodo luctus, nisi erat porttitor ligula, eget lacinia odio sem nec elit. Fusce dapibus, tellus ac cursus commodo, tortor mauris condimentum nibh, ut fermentum massa justo sit amet risus. Integer posuere erat a ante venenatis dapibus posuere velit aliquet. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus."));
				fpsLabel->SetFrame(Rect(0.0f, 0.0f, 250.0f, 24.0f));
				fpsLabel->SetTextColor(Color::Black());
				fpsLabel->SetAlignment(TextAlignment::Center);
				
				_debugWidget = new Widget(Rect(10.0f, 10.0f, 250.0f, 180.0f));
				_debugWidget->ContentView()->AddSubview(fpsLabel);
				
				fpsLabel->Release();
			}
			
			return _debugWidget;
		}
	}
}
