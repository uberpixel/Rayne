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
#include "RNImageView.h"

namespace RN
{
	Application::Application()
	{
		ImageView *imageView = new ImageView();
		imageView->SetFrame(Rect(50.0f, 50.0f, 100.0f, 100.0f));
		imageView->SetImage(Image::WithFile("textures/rn_Default.png"));
		
		Widget *widget = new Widget(Rect(100.0f, 100.0f, 200.0f, 200.0f));
		widget->Show();
		widget->ContentView()->AddSubview(imageView);
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
