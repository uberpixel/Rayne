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

		if(RN::Kernel::SharedInstance()->ScaleFactor() <= 1.4f)
		{
			window->SetConfiguration(RN::WindowConfiguration(1680, 1050), 0);
		}
		else
		{
			window->SetConfiguration(RN::WindowConfiguration(960, 600), 0);
		}
		
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
