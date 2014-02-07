//
//  TGApplication.cpp
//  Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "TGApplication.h"
#include "TGLoadingScreen.h"

#include "TGForestWorld.h"
#include "TGSponzaWorld.h"

namespace TG
{
	Application::Application()
	{
		SetTitle("Super Awesome Game");
	}
	
	Application::~Application()
	{}
	
	
	
	void Application::Start()
	{
		RN::Texture::SetDefaultAnisotropyLevel(RN::Texture::GetMaxAnisotropyLevel());
		RN::Kernel::GetSharedInstance()->SetMaxFPS(60);
		
		RN::UI::Widget *widget;/* = new RN::UI::DebugWidget();
		widget->Open();
		widget->Release();*/
		
		/*widget = new RN::UI::ConsoleWidget();
		widget->Open();
		widget->Release();*/

		RN::World *world = new ForestWorld();
		RN::Progress *progress = RN::WorldCoordinator::GetSharedInstance()->LoadWorld(world->Autorelease());
		
		widget = new LoadingScreen(progress);
		widget->Open();
	}
	
	void Application::WillExit()
	{
	}
	
	
	
	void Application::GameUpdate(float delta)
	{
	}
	void Application::WorldUpdate(float delta)
	{
	}
}
