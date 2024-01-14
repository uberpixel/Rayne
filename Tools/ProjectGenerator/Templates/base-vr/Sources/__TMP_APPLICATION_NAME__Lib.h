//
//  __TMP_APPLICATION_NAME__Lib.h
//  __TMP_APPLICATION_NAME__
//
//  Copyright __TMP_YEAR__ by __TMP_COMPANY__. All rights reserved.
//

#ifndef __TMP_APPLICATION_NAME___LIB_H
#define __TMP_APPLICATION_NAME___LIB_H

#if RN_PLATFORM_IOS || RN_PLATFORM_VISIONOS
#import <Foundation/Foundation.h>
#endif

#include "__TMP__Application.h"
#include "__TMP__CameraManager.h"
#include "__TMP__Types.h"
#include "__TMP__World.h"

#if RN_PLATFORM_VISIONOS
	#import <CompositorServices/CompositorServices.h>
	void visionos_main(cp_layer_renderer_t layerRenderer);
#elif RN_PLATFORM_IOS
	#import <UIKit/UIKit.h>
	void ios_main(CAMetalLayer *view);
#endif

#endif //__TMP_APPLICATION_NAME___LIB_H
