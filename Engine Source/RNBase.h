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

// ---------------------------
// Platform dependent includes
// ---------------------------
#if RN_PLATFORM_POSIX
	#include <pthread.h>
	#include <signal.h>
	#include <errno.h>
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
}

#endif /* __RAYNE_BASE_H__ */
