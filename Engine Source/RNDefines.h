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

	#define M_E         2.71828182845904523536028747135266250f
	#define M_LOG2E     1.44269504088896340735992468100189214f
	#define M_LOG10E    0.434294481903251827651128918916605082f
	#define M_LN2       0.693147180559945309417232121458176568f
	#define M_LN10      2.30258509299404568401799145468436421f
	#define M_PI        3.14159265358979323846264338327950288f
	#define M_PI_2      1.57079632679489661923132169163975144f
	#define M_PI_4      0.785398163397448309615660845819875721f
	#define M_1_PI      0.318309886183790671537767526745028724f
	#define M_2_PI      0.636619772367581343075535053490057448f
	#define M_2_SQRTPI  1.12837916709551257389615890312154517f
	#define M_SQRT2     1.41421356237309504880168872420969808f
	#define M_SQRT1_2   0.707106781186547524400844362104849039f

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
