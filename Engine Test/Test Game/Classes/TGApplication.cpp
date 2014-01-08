//
//  TGApplication.cpp
//  Game
//
//  Created by Sidney Just on 27.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "TGApplication.h"
#include "TGLoadingScreen.h"

namespace TG
{
	Application::Application()
	{}
	
	Application::~Application()
	{}
	
	
	
	void Application::Start()
	{
		RN::Texture::SetDefaultAnisotropyLevel(RN::Texture::GetMaxAnisotropyLevel());
		SetTitle("Super Awesome Game");
		
		RN::Kernel::GetSharedInstance()->SetMaxFPS(60);
		
		RN::UI::Widget *widget;/* = new RN::UI::DebugWidget();
		widget->Open();
		widget->Release();*/
		
		/*widget = new RN::UI::ConsoleWidget();
		widget->Open();
		widget->Release();*/

		RN::World *world = new World();
		RN::Progress *progress = RN::WorldCoordinator::GetSharedInstance()->LoadWorld(world->Autorelease());
		
		widget = new LoadingScreen(progress);
		widget->Open();
	}
	
	void Application::WillExit()
	{
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
