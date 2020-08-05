//
//  __TMP__Application.cpp
//  __TMP_APPLICATION_NAME__
//
//  Copyright __TMP_YEAR__ by __TMP_COMPANY__. All rights reserved.
//

#include "__TMP__Application.h"
#include "__TMP__World.h"
#include "RNVRApplicationImpl.h"

namespace __TMP__
{
	Application::Application()
	{
		
	}

	Application::~Application()
	{
		
	}


	void Application::WillFinishLaunching(RN::Kernel *kernel)
	{
		RN::Application::WillFinishLaunching(kernel);
		if(!RN::Kernel::GetSharedInstance()->GetArguments().HasArgument("pancake", '2d'))
		{
			SetupVR();
		}
	}

	void Application::DidFinishLaunching(RN::Kernel *kernel)
	{
		RN::VRApplication::DidFinishLaunching(kernel);
		
#if RN_PLATFORM_ANDROID
		RN::Shader::Sampler::SetDefaultAnisotropy(4);
#else
		RN::Shader::Sampler::SetDefaultAnisotropy(16);
#endif

		World *world = new World(GetVRWindow());
		RN::SceneManager::GetSharedInstance()->AddScene(world->Autorelease());
	}
}
