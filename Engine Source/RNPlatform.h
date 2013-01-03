//
//  RNPlatform.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PLATFORM_H__
#define __RAYNE_PLATFORM_H__

#define RN_PLATFORM_MAC_OS  0
#define RN_PLATFORM_WINDOWS 0
#define RN_PLATFORM_IOS     0

#define RN_PLATFORM_ARM   0
#define RN_PLATFORM_INTEL 0

#define RN_PLATFORM_32BIT 0
#define RN_PLATFORM_64BIT 0

#define RN_TARGET_OPENGL    0
#define RN_TARGET_OPENGL_ES 0

#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) || defined(__IPHONE_OS_VERSION_MAX_ALLOWED)
	#define RN_PLATFORM_POSIX 1

	#ifdef __MAC_OS_X_VERSION_MAX_ALLOWED
		#undef RN_PLATFORM_MAC_OS
		#undef RN_PLATFORM_INTEL
		#undef RN_PLATFORM_64BIT
		#undef RN_TARGET_OPENGL

		#define RN_PLATFORM_MAC_OS 1
		#define RN_PLATFORM_INTEL 1
		#define RN_PLATFORM_64BIT 1
		#define RN_TARGET_OPENGL 1
	#else
		#undef RN_PLATFORM_IOS
		#undef RN_PLATFORM_ARM
		#undef RN_PLATFORM_32BIT
		#undef RN_TARGET_OPENGL_ES

		#define RN_PLATFORM_IOS 1
		#define RN_PLATFORM_ARM 1
		#define RN_PLATFORM_32BIT 1
		#define RN_TARGET_OPENGL_ES 1
	#endif
#endif

#if defined(_MSC_VER)
	#undef RN_PLATFORM_WINDOWS
	#undef RN_TARGET_OPENGL

	#define RN_PLATFORM_WINDOWS 1
	#define RN_TARGET_OPENGL 1

	#pragma section(".CRT$XCU",read)
	#pragma warning(disable: 4800)

	

	#if defined(_WIN64) || defined(__amd64__)
			#undef RN_PLATFORM_INTEL
			#undef RN_PLATFORM_64BIT

			#define RN_PLATFORM_INTEL 1
			#define RN_PLATFORM_64BIT 1
		#else
			#undef RN_PLATFORM_INTEL
			#undef RN_PLATFORM_32BIT

			#define RN_PLATFORM_INTEL 1
			#define RN_PLATFORM_32BIT 1
	#endif
#endif

#endif /* __RAYNE_PLATFORM_H__ */
