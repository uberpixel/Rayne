//
//  RNUIWindow.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIWindow.h"
#include "RNUIServer.h"
#include "RNUIView.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Window, View)

		Window::Window(const Rect &frame) :
			View(frame),
			_server(nullptr)
		{
			SetDepthModeAndWrite(DepthMode::Always, false, 0.0f, 0.0f);
		}

		Window::~Window()
		{
			
		}

		void Window::Open(Server *server)
		{
			if(!server)
				server = Server::GetDefaultServer();

			server->AddWindow(this);
		}
		void Window::Close()
		{
			if(_server)
				_server->RemoveWindow(this);
		}

		void Window::Update(float delta)
		{
			Draw(false);
		}
	}
}
