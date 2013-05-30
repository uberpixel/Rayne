//
//  RNApplication.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNApplication.h"

#include "RNThreadPool.h"
#include "RNUIServer.h"
#include "RNUIImageView.h"

#include "Rayne.h"

namespace RN
{
	Application::Application()
	{}
	
	Application::~Application()
	{}
	
	
	
	void Application::Start()
	{}
	
	bool Application::CanExit()
	{
		return true;
	}
	
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
