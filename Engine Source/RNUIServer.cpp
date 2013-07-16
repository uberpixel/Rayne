//
//  RNUIServer.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIServer.h"
#include "RNKernel.h"
#include "RNWorld.h"
#include "RNWindow.h"
#include "RNUILabel.h"
#include "RNUIButton.h"

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
			_mode = Mode::SingleTracking;
			
			_debugWidget = nullptr;
			_drawDebugFrames = true;
			
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
		
		
		void Server::HandleEvent(Message *message)
		{
			Event *event = static_cast<Event *>(message);
			Responder *responder = Responder::FirstResponder();
			
			if(event->IsMouse())
			{
				if(responder && event->EventType() == Event::Type::MouseMoved)
				{
					responder->MouseMoved(event);
					return;
				}
				
				const Vector2& position = event->MousePosition();
				View *hit = nullptr;
				
				for(Widget *widget : _widgets)
				{
					if(widget->Frame().ContainsPoint(position))
					{
						Vector2 transformed = position;
						transformed.x -= widget->_frame.x;
						transformed.y -= widget->_frame.y;
						
						hit = widget->ContentView()->HitTest(transformed, event);
						break;
					}
				}
				
				if(hit)
				{
					switch(event->EventType())
					{
						case Event::Type::MouseWheel:
							hit->ScrollWheel(event);
							break;
							
						case Event::Type::MouseDown:
							hit->MouseDown(event);
							break;
							
						case Event::Type::MouseUp:
							hit->MouseUp(event);
							break;
							
						default:
							break;
					}
				}
			}
			
			if(responder && event->IsKeyboard())
			{
				switch(event->EventType())
				{
					case Event::Type::KeyDown:
						responder->KeyDown(event);
						break;
						
					case Event::Type::KeyUp:
						responder->KeyUp(event);
						break;
						
					case Event::Type::KeyRepeat:
						responder->KeyRepeat(event);
						break;
						
					default:
						break;
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
