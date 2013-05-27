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
/*		const std::vector<RN::WindowConfiguration>& configurations = window->Configurations();
		
		size_t index = (RN::Kernel::SharedInstance()->ScaleFactor() <= 1.4f) ? (configurations.size() - 2) : 0;

		RN::Window::SharedInstance()->SetConfiguration(configurations.at(index), 0);*/
		//RN::Window::SharedInstance()->HideCursor();
		
		RN::Window::SharedInstance()->SetConfiguration(RN::WindowConfiguration(840, 525), 0);
		
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
