//
//  RNDefines.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_DEFINES_H__
#define __RAYNE_DEFINES_H__

#include <type_traits>

#define RN_PLATFORM_MAC_OS  0
#define RN_PLATFORM_WINDOWS 0
#define RN_PLATFORM_IOS     0
#define RN_PLATFORM_LINUX	0

#define RN_PLATFORM_ARM   0
#define RN_PLATFORM_INTEL 0

#define RN_PLATFORM_32BIT 0
#define RN_PLATFORM_64BIT 0

#define RN_TARGET_OPENGL    0
#define RN_TARGET_OPENGL_ES 0

#define RN_TARGET_CXX_NOXCEPT   0
#define RN_TARGET_CXX_CONSTEXPR 0

#define RN_BUILD_DEBUG   1
#define RN_BUILD_RELEASE 1

#if DEBUG
	#undef RN_BUILD_RELEASE
	#define RN_BUILD_RELEASE 0
#endif /* DEBUG */
#if NDEBUG
	#undef RN_BUILD_DEBUG
	#define RN_BUILD_DEBUG 0
#endif /* NDEBUG */

#if defined(__APPLE__) && defined(__MACH__)

	#include <TargetConditionals.h>
	#define RN_PLATFORM_POSIX 1

	#ifdef TARGET_OS_MAC
		#undef RN_PLATFORM_MAC_OS
		#undef RN_PLATFORM_INTEL
		#undef RN_TARGET_OPENGL

		#define RN_PLATFORM_MAC_OS 1
		#define RN_PLATFORM_INTEL  1
		#define RN_TARGET_OPENGL   1
	#else
		#undef RN_PLATFORM_IOS
		#undef RN_PLATFORM_ARM
		#undef RN_TARGET_OPENGL_ES

		#define RN_PLATFORM_IOS     1
		#define RN_PLATFORM_ARM     1
		#define RN_TARGET_OPENGL_ES 1
	#endif /* TARGET_OS_MAC */

	#ifdef __LP64__
		#undef  RN_PLATFORM_64BIT
		#define RN_PLATFORM_64BIT 1
	#else
		#undef  RN_PLATFORM_32BIT
		#define RN_PLATFORM_32BIT 1
	#endif /* __LP64__ */
#endif /* defined(__APPLE__) && defined(__MACH__) */

#if defined(_WIN32)
	#undef RN_PLATFORM_WINDOWS
	#undef RN_TARGET_OPENGL

	#define RN_PLATFORM_WINDOWS 1
	#define RN_TARGET_OPENGL    1

	#if defined(_WIN64)
		#undef RN_PLATFORM_INTEL
		#undef RN_PLATFORM_64BIT

		#define RN_PLATFORM_INTEL 1
		#define RN_PLATFORM_64BIT 1
	#else
		#undef RN_PLATFORM_INTEL
		#undef RN_PLATFORM_32BIT

		#define RN_PLATFORM_INTEL 1
		#define RN_PLATFORM_32BIT 1
	#endif /* defined(_WIN64)*/
#endif /* defined(_WIN32) */

#if defined(__linux__)
	#define RN_PLATFORM_POSIX 1

	#undef RN_PLATFORM_LINUX
	#undef RN_TARGET_OPENGL

	#define RN_PLATFORM_LINUX 1
	#define RN_TARGET_OPENGL 1

	#if defined(__x86_64__)
		#undef RN_PLATFORM_INTEL
		#undef RN_PLATFORM_64BIT

		#define RN_PLATFORM_INTEL 1
		#define RN_PLATFORM_64BIT 1
	#else
		#undef RN_PLATFORM_INTEL
		#undef RN_PLATFORM_32BIT

		#define RN_PLATFORM_INTEL 1
		#define RN_PLATFORM_32BIT 1
	#endif /* defined(_x86_64__) */
#endif /* defined(__linux__) */



#if defined(_MSC_VER)

	typedef signed char				int8;
	typedef unsigned char			uint8;

	typedef short					int16;
	typedef unsigned short			uint16;

	typedef int						int32;
	typedef unsigned int			uint32;

	typedef __int64					int64;
	typedef unsigned __int64		uint64;

	#define alignas(n) __declspec(align(n))

	#pragma warning(disable: 4018)
	#pragma warning(disable: 4244)
	#pragma warning(disable: 4250)
	#pragma warning(disable: 4305)
	#pragma warning(disable: 4316)
	#pragma warning(disable: 4800)
	#pragma warning(disable: 4996)

	#define RN_FUNCTION_SIGNATURE __FUNCTION__
	#define RN_NORETURN __declspec(noreturn)
	#define RN_INLINE inline

	#if RN_BUILD_LIBRARY
		#define RNAPI __declspec(dllexport)
		#define RNAPI_EXPORTONLY __declspec(dllexport)
	#else
		#define RNAPI __declspec(dllimport)
		#define RNAPI_EXPORTONLY
	#endif /* RN_BUILD_LIBRARY */

	#if RN_BUILD_MODULE
		#define RNMODULEAPI __declspec(dllexport)
	#else
		#define RNMODULEAPI __declspec(dllimport)
	#endif /* RN_BUILD_MODULE */
#endif /* _MSC_VER */

#if defined(__GNUC__) // Also catches Clang on OS X

	#undef RN_TARGET_CXX_NOXCEPT
	#undef RN_TARGET_CXX_CONSTEXPR

	#define RN_TARGET_CXX_NOXCEPT   1
	#define RN_TARGET_CXX_CONSTEXPR 1

	typedef signed char				int8;
	typedef unsigned char			uint8;

	typedef short					int16;
	typedef unsigned short			uint16;

	typedef int						int32;
	typedef unsigned int			uint32;

	typedef long long				int64;
	typedef unsigned long long		uint64;

	#define RN_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
	#define RN_NORETURN __attribute__((noreturn))
	#define RN_INLINE inline __attribute__((__always_inline__))

	#if RN_BUILD_LIBRARY
		#define RNAPI __attribute__((visibility("default")))
		#define RNAPI_EXPORTONLY __attribute__((visibility("default")))
	#else
		#define RNAPI
		#define RNAPI_EXPORTONLY
	#endif /* RN_BUILD_LIBRARY */

	#if RN_BUILD_MODULE
		#define RNMODULEAPI __attribute__((visibility("default")))
	#else
		#define RNMODULEAPI
	#endif /* RN_BUILD_MODULE */
#endif /* __GNUC__ */

// Sanity checks
typedef size_t machine_hash;

#if (RN_BUILD_DEBUG && RN_BUILD_RELEASE) || (!RN_BUILD_DEBUG && !RN_BUILD_RELEASE)
	#error "Debug and Release build both defined (only define either one of DEBUG or NDEBUG"
#endif

static_assert(sizeof(int8) == 1, "int8 must be 1 byte!");
static_assert(sizeof(int16) == 2, "int16 must be 2 bytes!");
static_assert(sizeof(int32) == 4, "int32 must be 4 bytes!");
static_assert(sizeof(int64) == 8, "int64 must be 8 bytes!");

static_assert(sizeof(uint8) == 1, "uint8 must be 1 byte!");
static_assert(sizeof(uint16) == 2, "uint16 must be 2 bytes!");
static_assert(sizeof(uint32) == 4, "uint32 must be 4 bytes!");
static_assert(sizeof(uint64) == 8, "uint64 must be 8 bytes!");

static_assert(std::is_signed<int8>::value, "int8 must be signed!");
static_assert(std::is_signed<int16>::value, "int16 must be signed!");
static_assert(std::is_signed<int32>::value, "int32 must be signed!");
static_assert(std::is_signed<int64>::value, "int64 must be signed!");

static_assert(std::is_unsigned<uint8>::value, "uint8 must be unsigned!");
static_assert(std::is_unsigned<uint16>::value, "uint16 must be unsigned!");
static_assert(std::is_unsigned<uint32>::value, "uint32 must be unsigned!");
static_assert(std::is_unsigned<uint64>::value, "uint64 must be unsigned!");

static_assert(std::is_same<std::size_t, machine_hash>::value, "size_t and machine_hash must be the same!");

#endif /* __RAYNE_DEFINES_H__ */
