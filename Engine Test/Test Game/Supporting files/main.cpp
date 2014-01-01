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
	char path[MAX_PATH + 1];
	::SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, path);

	std::stringstream stream;
	stream << path << "\\Rayne\\Engine Resources";
	
	RN::FileManager::GetSharedInstance()->AddSearchPath(stream.str());
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
