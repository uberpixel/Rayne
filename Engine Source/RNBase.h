//
//  RNBase.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
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

	#include <string>
	#include <vector>
#endif

#if RN_PLATFORM_MAC_OS
	#import <Cocoa/Cocoa.h>
	#include <OpenGL/OpenGL.h>
	#include <OpenGL/gl.h>
	#include <OpenGL/glext.h>

	#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
		#include <OpenGL/gl3.h>
		#include <OpenGL/gl3ext.h>
	#endif
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
	RN_EXTERN void Assert(bool condition, const char *message = 0);
	
#if RN_PLATFORM_MAC_OS
	#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_8
		static inline void OSXVersion(int32 *major, int32 *minor, int32 *patch)
		{
			NSDictionary *dict = [NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"];
			Assert(dict);
			
			NSArray *versionComponents = [dict objectForKey:@"ProductVersion"];
			Assert(dict && [dict count] == 3);
			
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
