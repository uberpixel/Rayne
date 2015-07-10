//
//  RNApplication.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNApplication.h"
#include "RNKernel.h"

namespace RN
{
	Application::Application() :
		_title(nullptr)
	{

	}

	Application::~Application()
	{
		SafeRelease(_title);
	}

	void Application::__PrepareForWillFinishLaunching(Kernel *kernel)
	{
		_title = kernel->GetManifestEntryForKey<String>(kRNManifestApplicationKey);
	}

	void Application::WillFinishLaunching(Kernel *kernel) {}
	void Application::DidFinishLaunching(Kernel *kernel) {}
	void Application::WillExit() {}

	void Application::WillStep(float delta) {}
	void Application::DidStep(float delta) {}

	void Application::WillBecomeActive() {}
	void Application::DidBecomeActive() {}
	void Application::WillResignActive() {}
	void Application::DidResignActive() {}
}
