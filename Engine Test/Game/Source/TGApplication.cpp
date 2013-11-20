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
		RN::Texture::SetDefaultAnisotropyLevel(RN::Texture::GetMaxAnisotropyLevel());
		
		SetTitle("Super Awesome Game");
	}
	
	Application::~Application()
	{}
	
	
	void Application::Start()
	{
		RN::Window *window = RN::Window::GetSharedInstance();
		RN::UI::DebugWidget *widget = new RN::UI::DebugWidget();
		widget->Show();
		
		auto configs = RN::Window::GetSharedInstance()->GetMainScreen()->GetConfigurations();
		window->SetConfiguration(new RN::WindowConfiguration(960, 600), 0);//configs.GetObjectAtIndex<RN::WindowConfiguration>(configs.GetCount() - 2), 0);
			
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
