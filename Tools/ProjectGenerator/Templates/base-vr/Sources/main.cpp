//
//  main.cpp
//  __TMP_APPLICATION_NAME__
//
//  Copyright __TMP_YEAR__ by __TMP_COMPANY__. All rights reserved.
//

#include <Rayne.h>
#include "__TMP__Application.h"

#if RN_BUILD_RELEASE
	#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#if RN_PLATFORM_ANDROID
	void android_main(struct android_app *app)
	{
		RN::Initialize(0, nullptr, new __TMP__::Application(), app);
	}
#else
	int main(int argc, const char *argv[])
	{
		#if RN_BUILD_DEBUG
		//	_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF);
		#endif
		
		RN::Initialize(argc, argv, new __TMP__::Application());
	}
#endif
