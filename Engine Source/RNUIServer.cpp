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
	UIServer::UIServer()
	{
		uint32 flags = Camera::FlagOrthogonal | Camera::FlagUpdateAspect | Camera::FlagUpdateStorageFrame;
		_camera = new Camera(Vector2(0.0f), TextureParameter::Format::RGBA8888, flags, RenderStorage::BufferFormatColor);
		_camera->SetAllowsDepthWrite(false);
		_camera->SetClearColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
		_camera->SetUseBlending(true);
		
		if(_camera->Container())
			_camera->Container()->RemoveSceneNode(_camera);
	}
	
	UIServer::~UIServer()
	{
		_camera->Release();
	}
	
	
	
	void UIServer::AddWidget(Widget *widget)
	{
		RN_ASSERT0(widget->_server == 0);
		
		_widgets.push_front(widget);
	}
	
	void UIServer::RemoveWidget(Widget *widget)
	{
		RN_ASSERT0(widget->_server == this);
		
		_widgets.erase(std::remove(_widgets.begin(), _widgets.end(), widget), _widgets.end());
	}
	
	void UIServer::MoveWidgetToFront(Widget *widget)
	{
		RN_ASSERT0(widget->_server == this);
		
		_widgets.erase(std::remove(_widgets.begin(), _widgets.end(), widget), _widgets.end());
		_widgets.push_front(widget);
	}
	
	
	void UIServer::Render(Renderer *renderer)
	{
		if(_widgets.size() == 0)
			return;
		
		Rect actualFrame = Window::SharedInstance()->Frame();
		if(_frame != actualFrame)
		{
			_frame = actualFrame;
			_camera->SetFrame(_frame);
		}
		
		
		renderer->BeginCamera(_camera);
		
		for(Widget *widget : _widgets)
		{
			widget->Render(renderer);
		}
		
		renderer->FinishCamera();
	}
}
