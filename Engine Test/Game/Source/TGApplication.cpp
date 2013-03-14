//
//  TGApplication.cpp
//  Game
//
//  Created by Sidney Just on 27.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "TGApplication.h"

namespace TG
{
	Application::Application()
	{
		_world = 0;
		RN::Texture::SetDefaultAnisotropyLevel(RN::Texture::MaxAnisotropyLevel());
		
		SetTitle("Super Awesome Game");
	}
	
	Application::~Application()
	{}
	
	
	void Application::Start()
	{
		RN::Window *window = RN::Window::SharedInstance();
		const RN::Array<RN::WindowConfiguration *>& configurations = window->Configurations();
		
		RN::WindowConfiguration *configuration = configurations.LastObject();
		
		RN::Window::SharedInstance()->SetConfiguration(configuration, RN::Window::WindowMaskFullscreen | RN::Window::WindowMaskVSync);
		RN::Window::SharedInstance()->HideCursor();
		
		_world = new World();
	}
	
	void Application::WillExit()
	{
		if(_world)
		{
			delete _world;
			_world = 0;
		}
	}
	
	
	void Application::UpdateGame(float delta)
	{
	}
	
	void Application::UpdateWorld(float delta)
	{
	}
}

extern "C"
{
	RN::Application *RNApplicationCreate(RN::Kernel *kernel)
	{
		return static_cast<RN::Application *>(new TG::Application());
	}
}
