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
	}
	
	Application::~Application()
	{}
	
	
	void Application::Start()
	{
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
