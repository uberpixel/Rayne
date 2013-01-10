//
//  RNDefines.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_DEFINES_H__
#define __RAYNE_DEFINES_H__

#include "RNPlatform.h"

#if defined(_MSC_VER)

	typedef signed char				int8;
	typedef unsigned char			uint8;

	typedef short					int16;
	typedef unsigned short			uint16;

	typedef int						int32;
	typedef unsigned int			uint32;

	typedef __int64					int64;
	typedef unsigned __int64		uint64;

	#define RN_INITIALIZER(f) \
		static void __cdecl f(); \
		__declspec(allocate(".CRT$XCU")) void (__cdecl*f##_)(void) = f; \
		static void __cdecl f()

	#define RN_NORETURN __declspec(noreturn)

#elif defined(__GNUC__)

	typedef signed char				int8;
	typedef unsigned char			uint8;

	typedef short					int16;
	typedef unsigned short			uint16;

	typedef int						int32;
	typedef unsigned int			uint32;

	typedef long long				int64;
	typedef unsigned long long		uint64;

	#define RN_INITIALIZER(f) \
		static void f() __attribute__((constructor)); \
		static void f()

	#define RN_NORETURN __attribute__((noreturn))

#else
	#error "Unsupported compiler."
#endif

#if RN_PLATFORM_32BIT

	typedef int32 machine_int;
	typedef uint32 machine_uint;

	typedef uint32 machine_hash;

#elif RN_PLATFORM_64BIT

	typedef int64 machine_int;
	typedef uint64 machine_uint;

	typedef uint64 machine_hash;

#else
	#error Unknown platform
#endif

#endif /* __RAYNE_DEFINES_H__ */
