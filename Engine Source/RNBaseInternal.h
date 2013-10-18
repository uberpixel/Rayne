//
//  RNBaseInternal.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BASEINTERNAL_H__
#define __RAYNE_BASEINTERNAL_H__

/**
 * This header is for INTERNAL use only!
 * Don't include it from other .h files!
 **/

#include "RNBase.h"

#if RN_PLATFORM_IOS
	#import <UIKit/UIKit.h>
	#import <QuartzCore/QuartzCore.h>
#endif

#if RN_PLATFORM_MAC_OS
	#import <Cocoa/Cocoa.h>
	#import <OpenGL/OpenGL.h>
	#import <CoreText/CoreText.h>
	#import <Foundation/Foundation.h>
	#import <CoreServices/CoreServices.h>
#endif

#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS
	#import <objc/objc-runtime.h>
#endif

#endif /* __RAYNE_BASEINTERNAL_H__ */
