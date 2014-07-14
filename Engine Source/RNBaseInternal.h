//
//  RNBaseInternal.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BASEINTERNAL_H__
#define __RAYNE_BASEINTERNAL_H__

/**
 * This header is for INTERNAL use only!
 * Don't include it from other .h files!
 **/

#include "RNBase.h"
#include "RNLogging.h"

#if RN_PLATFORM_IOS
	#import <UIKit/UIKit.h>
	#import <QuartzCore/QuartzCore.h>
#endif

#if RN_PLATFORM_MAC_OS
	#define GL_GLEXT_FUNCTION_POINTERS 1

	#import <Cocoa/Cocoa.h>
	#import <CoreText/CoreText.h>
	#import <Foundation/Foundation.h>
	#import <CoreServices/CoreServices.h>

	#import <IOKit/IOKitLib.h>
	#import <IOKit/IOCFPlugIn.h>
	#import <IOKit/hid/IOHIDLib.h>
	#import <IOKit/usb/IOUSBLib.h>
#endif

#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS
	#import <objc/objc-runtime.h>
#endif

#if RN_PLATFORM_POSIX
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/sysctl.h>
	#include <unistd.h>
#endif

#endif /* __RAYNE_BASEINTERNAL_H__ */
