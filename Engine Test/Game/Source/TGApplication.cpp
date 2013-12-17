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
	Application::Application() :
		_world(nullptr)
	{}
	
	Application::~Application()
	{}
	
	
	
	void Application::Start()
	{
		RN::Texture::SetDefaultAnisotropyLevel(RN::Texture::GetMaxAnisotropyLevel());
		SetTitle("Super Awesome Game");
		
		RN::Kernel::GetSharedInstance()->SetMaxFPS(60);
		
		RN::UI::DebugWidget *widget = new RN::UI::DebugWidget();
		widget->Show();
		widget->Release();

		_world = new World();
	}
	
	void Application::WillExit()
	{
		delete _world;
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
