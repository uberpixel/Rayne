//
//  RNApplication.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNApplication.h"

namespace RN
{
	RNDeclareSingleton(Application)
	
	Application::Application()
	{
		MakeShared();
	}
	
	Application::~Application()
	{}
	
	
	
	void Application::Start()
	{}

	void Application::WillExit()
	{}
	
	
	
	void Application::GameUpdate(float delta)
	{
	}
	
	void Application::WorldUpdate(float delta)
	{
	}
	
	void Application::SetTitle(const std::string& title)
	{
		_title = title;
	}
}
