//
//  RNBase.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
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
#include <math.h>

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <type_traits>
#include <tuple>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <unordered_set>

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
	#include <dlfcn.h>
	#include <stdarg.h>
#endif

#if RN_PLATFORM_MAC_OS
	#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED 1

	#include <libkern/OSAtomic.h>
	#include <tmmintrin.h>

	#include <IOKit/IOKitLib.h>
	#include <IOKit/IOCFPlugIn.h>
	#include <IOKit/hid/IOHIDBase.h>
	#include <IOKit/hid/IOHIDKeys.h>
	#include <IOKit/hid/IOHIDUsageTables.h>
	#include <IOKit/hid/IOHIDLib.h>

	#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
		#include <OpenGL/gl3.h>
		#include <OpenGL/gl3ext.h>
	#else
		#include <OpenGL/gl.h>
		#include <OpenGL/glext.h>
	#endif
#endif

#if RN_PLATFORM_IOS
	#include <OpenGLES/ES2/gl.h>
	#include <OpenGLES/ES2/glext.h>

	#include <libkern/OSAtomic.h>
#endif

#if RN_PLATFORM_WINDOWS
	#define WINDOWS_LEAN_AND_MEAN // fuck MFC!
	#include <windows.h>

	#include "gl11.h"
	#include "glext.h"
	#include "wglext.h"

	#pragma comment(lib, "opengl32.lib")
#endif

#if RN_PLATFORM_LINUX

	#define GL_GLEXT_PROTOTYPES 1
	#define GL3_PROTOTYPES 1

	#include <GL/glx.h>
	#include <GL/gl.h>
	//#include <GL/glu.h>
	//#include <GL/glut.h>
#endif

#include "RNOpenGL.h"

// ---------------------------
// Helper macros
// ---------------------------
#define kRNEpsilonFloat 0.001f

typedef int8 ComparisonResult;

#define kRNCompareLessThan     -1
#define kRNCompareEqualTo       0
#define kRNCompareGreaterThan   1

#define RN_INLINE inline
#define RN_EXTERN extern

#define RN_NOT_FOUND ((machine_uint)-1)

#ifndef MAX
#define MAX(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#endif

#ifndef MIN
#define MIN(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _b : _a; })
#endif

namespace RN
{
#ifndef NDEBUG
	RNAPI RN_NORETURN void __Assert(const char *func, int line, const char *expression, const char *message, ...);

	#if RN_PLATFORM_POSIX
		#define RN_ASSERT(e, ...) __builtin_expect(!(e), 0) ? __Assert(__func__, __LINE__, #e, __VA_ARGS__) : (void)0
		#define RN_ASSERT0(e) __builtin_expect(!(e), 0) ? __Assert(__func__, __LINE__, #e, 0) : (void)0
	#endif

	#if RN_PLATFORM_WINDOWS
		#define RN_ASSERT(e, ...) (!(e)) ? __Assert(__FUNCTION__, __LINE__, #e, __VA_ARGS__) : (void)0
		#define RN_ASSERT0(e) (!(e)) ? __Assert(__FUNCTION__, __LINE__, #e, 0) : (void)0
	#endif

#else
	#define RN_ASSERT(e, message, ...) (void)0
	#define RN_ASSERT0(e) (void)0
#endif

	template <class T>
	class Singleton
	{
	public:
		static T *SharedInstance()
		{
			if(!_instance)
				_instance = new T();

			return _instance;
		}

	protected:
		Singleton()
		{}

		virtual ~Singleton()
		{
			if(_instance == this)
				_instance = 0;
		}

	private:
		static T *_instance;
	};

	template <class T>
	T * Singleton<T>::_instance = 0;

	template <class T>
	class NonConstructingSingleton
	{
	public:
		static T *SharedInstance()
		{
			return _instance;
		}

	protected:
		NonConstructingSingleton()
		{
			RN_ASSERT0(_instance == 0);
			_instance = static_cast<T *>(this);
		}

		virtual ~NonConstructingSingleton()
		{
			_instance = 0;
		}

	private:
		static T *_instance;
	};

	template <class T>
	T * NonConstructingSingleton<T>::_instance = 0;
}

#endif /* __RAYNE_BASE_H__ */
