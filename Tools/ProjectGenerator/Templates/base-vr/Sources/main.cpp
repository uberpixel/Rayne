//
//  main.cpp
//  __TMP_APPLICATION_NAME__
//
//  Copyright __TMP_YEAR__ by __TMP_COMPANY__. All rights reserved.
//

#include <Rayne.h>
#include "__TMP_APPLICATION_TARGET__Lib.h"

#if RN_BUILD_RELEASE
	#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#if RN_PLATFORM_VISIONOS
	void visionos_main(cp_layer_renderer_t layerRenderer)
	{
		RN::Initialize(0, nullptr, new __TMP__::Application(), layerRenderer);
	}
#elif RN_PLATFORM_IOS
	void ios_main(CAMetalLayer *metalLayer)
	{
		RN::Initialize(0, nullptr, new __TMP__::Application(), metalLayer);
	}
#elif RN_PLATFORM_ANDROID
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
