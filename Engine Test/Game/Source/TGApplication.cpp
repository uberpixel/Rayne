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
		_world = new World();
	}
	
	Application::~Application()
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
	void RNApplicationCreate(RN::Kernel *kernel)
	{
		new TG::Application();
	}
}
