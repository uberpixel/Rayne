//
//  main.cpp
//  Test Game
//
//  Created by Sidney Just on 31/12/13.
//  Copyright (c) 2013 Überpixel. All rights reserved.
//

#include <Rayne.h>
#include "TGApplication.h"

int main(int argc, char *argv[])
{
	RN::Initialize(argc, argv);
	
#if RN_PLATFORM_MAC_OS
	RN::FileManager::GetSharedInstance()->AddSearchPath("/usr/local/opt/Rayne/Engine Resources");
#endif
#if RN_PLATFORM_WINDOWS
	RN::FileManager::GetSharedInstance()->AddSearchPath("!!!ASD!!!XYZ!!!");
#endif
	
	try
	{
		auto application = new TG::Application();
		auto kernel = new RN::Kernel(application);
		
		while(kernel->Tick())
		{}
		
		delete kernel;
		delete application;
	}
	catch(RN::Exception e)
	{
		RN::HandleException(e);
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
