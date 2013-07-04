//
//  RNDefines.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_DEFINES_H__
#define __RAYNE_DEFINES_H__

#include <type_traits>
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

	#define RN_NORETURN __declspec(noreturn)
	#define RN_INLINE inline

	#if RN_BUILD_LIBRARY
		#define RNAPI __declspec(dllexport)
	#else
		#define RNAPI __declspec(dllimport)
	#endif

#elif defined(__GNUC__)

	typedef signed char				int8;
	typedef unsigned char			uint8;

	typedef short					int16;
	typedef unsigned short			uint16;

	typedef int						int32;
	typedef unsigned int			uint32;

	typedef long long				int64;
	typedef unsigned long long		uint64;

	#define RN_NORETURN __attribute__((noreturn))
	#define RN_INLINE inline __attribute__((__always_inline__))

	#if RN_BUILD_LIBRARY
		#define RNAPI __attribute__((visibility("default")))
	#else
		#define RNAPI
	#endif

#else
	#error "Unsupported compiler."
#endif

#if RN_PLATFORM_32BIT

	typedef size_t machine_hash;

#elif RN_PLATFORM_64BIT

	typedef size_t machine_hash;

#else
	#error Unknown platform
#endif

static_assert(sizeof(int8) == 1, "int8 must be 1 byte!");
static_assert(sizeof(int16) == 2, "int16 must be 2 bytes!");
static_assert(sizeof(int32) == 4, "int32 must be 4 bytes!");
static_assert(sizeof(int64) == 8, "int64 must be 4 bytes!");

static_assert(sizeof(uint8) == 1, "uint8 must be 1 byte!");
static_assert(sizeof(uint16) == 2, "uint16 must be 2 bytes!");
static_assert(sizeof(uint32) == 4, "uint32 must be 4 bytes!");
static_assert(sizeof(uint64) == 8, "uint64 must be 4 bytes!");

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
