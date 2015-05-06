//
//  RNApplication.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNApplication.h"

namespace RN
{
	Application::Application() :
		_title(nullptr)
	{
		SetTitle(RNCSTR(""));
	}

	Application::~Application()
	{
		SafeRelease(_title);
	}

	void Application::WillFinishLaunching(Kernel *kernel) {}
	void Application::DidFinishLaunching(Kernel *kernel) {}
	void Application::WillExit() {}

	void Application::Step(float delta) {}

	void Application::WillBecomeActive() {}
	void Application::DidBecomeActive() {}
	void Application::WillResignActive() {}
	void Application::DidResignActive() {}

	void Application::SetTitle(const String *title)
	{
		RN_ASSERT(title, "Title mustn't be NULL");

		SafeRelease(_title);
		_title = title->Copy();
	}
}
