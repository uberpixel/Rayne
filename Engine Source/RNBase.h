//
//  RNBase.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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

#include <type_traits>
#include <algorithm>
#include <string>
#include <iterator>
#include <vector>
#include <map>
#include <list>
#include <deque>
#include <tuple>
#include <chrono>
#include <utility>
#include <regex>
#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <unordered_map>
#include <unordered_set>

#include "RNPlatform.h"
#include "RNDefines.h"
#include "RNConstants.h"
#include "RNException.h"

// ---------------------------
// Platform dependent includes
// ---------------------------
#if RN_PLATFORM_POSIX
	#include <pthread.h>
	#include <signal.h>
	#include <errno.h>
	#include <dlfcn.h>
	#include <stdarg.h>
	#include <unistd.h>
	#include <wordexp.h>
#endif

#if RN_PLATFORM_MAC_OS
	#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED 1

	#include <IOKit/IOKitLib.h>
	#include <IOKit/IOCFPlugIn.h>

	#include <IOKit/hid/IOHIDBase.h>
	#include <IOKit/hid/IOHIDKeys.h>
	#include <IOKit/hid/IOHIDUsageTables.h>
	#include <IOKit/hid/IOHIDLib.h>

	#include <IOKit/graphics/IOGraphicsLib.h>

	#include <CoreGraphics/CoreGraphics.h>

	#include <OpenGL/gl3.h>
	#include <OpenGL/gl3ext.h>
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

	#include <X11/extensions/Xrandr.h>
	#undef FilterNearest

	#include <GL/glx.h>
	#include "glext.h"
	
#endif

#include "RNOpenGL.h"
#include "RNMath.h"
#include "RNSIMD.h"
#include "RNSpinLock.h"

// ---------------------------
// Helper macros
// ---------------------------
typedef uint32 FrameID;
typedef int8 ComparisonResult;

#define kRNCompareLessThan     -1
#define kRNCompareEqualTo       0
#define kRNCompareGreaterThan   1

namespace RN
{
#ifndef NDEBUG
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

	RNAPI RN_NORETURN void __Assert(const char *func, int line, const char *expression, const char *message, ...);
	RNAPI RN_NORETURN void __HandleException(const Exception& e);
	RNAPI void ParseCommandLine(int argc, char *argv[]);
	
	class Range
	{
	public:
		Range() {}
		Range(machine_uint torigin, machine_uint tlength)
		{
			origin = torigin;
			length = tlength;
		}
		
		machine_uint End() const
		{
			return origin + length;
		}
		
		bool Contains(const Range& other)
		{
			return (other.origin >= origin && End() <= other.End());
		}
		
		bool Overlaps(const Range& other)
		{
			return (End() >= other.origin && origin <= other.End());
		}
		
		machine_uint origin;
		machine_uint length;
	};
	
	template <typename T>
	class Singleton
	{
	public:
		static T *SharedInstance()
		{
			_lock.Lock();
			
			if(!_instance)
				_instance = new T();

			_lock.Unlock();
			return _instance;
		}

	protected:
		Singleton()
		{}

		virtual ~Singleton()
		{
			_lock.Lock();
			
			if(_instance == this)
				_instance = 0;
			
			_lock.Unlock();
		}

	private:
		static T *_instance;
		static SpinLock _lock;
	};

	template <typename T>
	T * Singleton<T>::_instance = 0;
	template <typename T>
	SpinLock Singleton<T>::_lock;

	template <typename T>
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

	template <typename T>
	T * NonConstructingSingleton<T>::_instance = 0;
}

#endif /* __RAYNE_BASE_H__ */
