//
//  RNPlatform.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PLATFORM_H__
#define __RAYNE_PLATFORM_H__

#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) || defined(__IPHONE_OS_VERSION_MAX_ALLOWED)
	#define RN_PLATFORM_POSIX 1

	#ifdef __MAC_OS_X_VERSION_MAX_ALLOWED
		#define RN_PLATFORM_MAC_OS 1
		#define RN_PLATFORM_INTEL 1
		#define RN_PLATFORM_64BIT 1
	#else
		#define RN_PLATFORM_IOS 1
		#define RN_PLATFORM_ARM 1
		#define RN_PLATFORM_32BIT 1
	#endif
#endif

#if defined(_MSC_VER)
	#define RN_PLATFORM_WINDOWS 1

	#pragma section(".CRT$XCU",read)
	#pragma warning(disable: 4800)

	#if defined(_WIN64) || defined(__amd64__)
			#define RN_PLATFORM_WIN64 1
			#define RN_PLATFORM_INTEL 1
			#define RN_PLATFORM_64BIT 1
		#else
			#define RN_PLATFORM_WIN32 1
			#define RN_PLATFORM_INTEL 1
			#define RN_PLATFORM_32BIT 1
	#endif
#endif

#include <string>


#endif /* __RAYNE_PLATFORM_H__ */
