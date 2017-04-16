//
//  SGApplication.cpp
//  Sword Game
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "SGApplication.h"
#include "SGWorld.h"

namespace SG
{
	void Application::WillFinishLaunching(RN::Kernel *kernel)
	{
		RN::Application::WillFinishLaunching(kernel);
		RN::Shader::Sampler::SetDefaultAnisotropy(16);
	}

	void Application::DidFinishLaunching(RN::Kernel *kernel)
	{
		RN::Application::DidFinishLaunching(kernel);

		World *world = new World();
		RN::SceneManager::GetSharedInstance()->AddScene(world);
	}
}
