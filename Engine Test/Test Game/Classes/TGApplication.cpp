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
	Application::Application() :
		_currentLevel(0)
	{
		SetTitle("Super Awesome Game");
	}
	
	Application::~Application()
	{}
	
	
	void Application::LoadLevel(uint32 levelID)
	{
		if(levelID == _currentLevel)
			return;
		
		RN::World *world = nullptr;
		
		switch(levelID)
		{
			case 1:
				world = new ForestWorld();
				break;
				
			case 2:
				world = new SponzaWorld();
				break;
				
			default:
				break;
		}
		
		if(!world)
			return;
		
		_currentLevel = levelID;
		
		RN::Settings::GetSharedInstance()->SetUint32ForKey(RNCSTR("lastLevel"), levelID);
		RN::Progress *progress = RN::WorldCoordinator::GetSharedInstance()->LoadWorld(world->Autorelease());
		
		RN::UI::Widget *widget = new LoadingScreen(progress);
		widget->Open();
	}
	
	
	
	void Application::Start()
	{
		RN::Texture::SetDefaultAnisotropyLevel(RN::Texture::GetMaxAnisotropyLevel());
		RN::Kernel::GetSharedInstance()->SetMaxFPS(60);
		
		/*RN::UI::Widget *widget = new RN::UI::DebugWidget();
		widget->Open();
		widget->Release();*/
		
		/*widget = new RN::UI::ConsoleWidget();
		widget->Open();
		widget->Release();*/
		
		// Load the last loaded level, or the forst level
		uint32 level = RN::Settings::GetSharedInstance()->GetUint32ForKey(RNCSTR("lastLevel"), 1);
		LoadLevel(level);
		
		RN::MessageCenter::GetSharedInstance()->AddObserver(kRNInputEventMessage, [this](RN::Message *message) {
			
			RN::Event *event = static_cast<RN::Event *>(message);
			
			if(event->GetType() == RN::Event::Type::KeyDown)
			{
				char character = event->GetCharacter();
				
				if(character >= '0' && character <= '9')
				{
					uint32 level = character - '0';
					LoadLevel(level);
				}
			}
			
		}, this);
	}
	
	void Application::WillExit()
	{
		RN::MessageCenter::GetSharedInstance()->RemoveObserver(this);
	}
	
	
	
	void Application::GameUpdate(float delta)
	{
	}
	void Application::WorldUpdate(float delta)
	{
	}
}
