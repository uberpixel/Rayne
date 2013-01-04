//
//  RNBase.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BASE_H__
#define __RAYNE_BASE_H__

// ---------------------------
// Platform independent includes
// ---------------------------
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <type_traits>


#include "RNPlatform.h"
#include "RNDefines.h"
#include "RNError.h"

// ---------------------------
// Platform dependent includes
// ---------------------------
#if RN_PLATFORM_POSIX
	#include <pthread.h>
	#include <signal.h>
	#include <errno.h>
#endif

#if RN_PLATFORM_MAC_OS
	#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED 1

	#import <Cocoa/Cocoa.h>
	#import <OpenGL/OpenGL.h>

	#include <libkern/OSAtomic.h>

	#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
		#include <OpenGL/gl3.h>
		#include <OpenGL/gl3ext.h>
	#else
		#include <OpenGL/gl.h>
		#include <OpenGL/glext.h>
	#endif
#endif

#if RN_PLATFORM_IOS
	#import <UIKit/UIKit.h>
	#import <OpenGLES/ES2/gl.h>
	#import <OpenGLES/ES2/glext.h>
	#import <QuartzCore/QuartzCore.h>

	#include <libkern/OSAtomic.h>
#endif

#if RN_PLATFORM_WINDOWS
	#define WINDOWS_LEAN_AND_MEAN // fuck MFC!
	#include <windows.h>
#endif

// ---------------------------
// Helper macros
// ---------------------------
#define kRNEpsilonFloat 0.001f

#define RN_INLINE inline
#define RN_EXTERN extern

#define RN_NOT_FOUND ((machine_uint)-1)

namespace RN
{
#ifndef NDEBUG
	RN_EXTERN void __Assert(const char *func, int line, const char *expression, const char *message, ...);
	
	#define RN_ASSERT(e, ...) __builtin_expect(!(e), 0) ? __Assert(__func__, __LINE__, #e, __VA_ARGS__) : (void)0
	#define RN_ASSERT0(e) if(!(e)) __builtin_expect(!(e), 0) ? __Assert(__func__, __LINE__, #e, 0) : (void)0
	
#else
	#define RN_ASSERT(e, message, ...) (void)0
#endif
	
#if RN_PLATFORM_MAC_OS
	#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_8
		static inline void OSXVersion(int32 *major, int32 *minor, int32 *patch)
		{
			NSDictionary *dict = [NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"];
			RN_ASSERT0(dict);
			
			NSArray *versionComponents = [[dict objectForKey:@"ProductVersion"] componentsSeparatedByString:@"."];
			RN_ASSERT0(dict);
			
			if(major)
				*major = [[versionComponents objectAtIndex:0] intValue];
			
			if(minor)
				*minor = [[versionComponents objectAtIndex:1] intValue];
			
			if(patch)
				*patch = [[versionComponents objectAtIndex:2] intValue];
		}
	#else
		static inline void OSXVersion(int32 *major, int32 *minor, int32 *patch)
		{
			if(major)
				Gestalt(gestaltSystemVersionMajor, major);
			
			if(minor)
				Gestalt(gestaltSystemVersionMinor, minor);
			
			if(patch)
				Gestalt(gestaltSystemVersionBugFix, patch);
		}
	#endif
#endif
	
	template <class T>
	class Singleton
	{
	public:
		static T *SharedInstance()
		{
			return 0;
		}
	};
}

#endif /* __RAYNE_BASE_H__ */
