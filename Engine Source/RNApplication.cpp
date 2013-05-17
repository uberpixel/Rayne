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

namespace RN
{
	Application::Application()
	{
/*		UI::ImageView *imageView = new UI::ImageView();
		imageView->SetFrame(Rect(50.0f, 50.0f, 96.0f, 28.0f));
		imageView->SetImage(UI::Image::WithFile("textures/button.png"));
		
		UI::Widget *widget = new UI::Widget(Rect(100.0f, 100.0f, 200.0f, 200.0f));
		widget->Show();
		widget->ContentView()->AddSubview(imageView);*/
	}
	
	Application::~Application()
	{
	}
	
	
	
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
