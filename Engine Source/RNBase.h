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
#include "RNMemory.h"

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <type_traits>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <sstream>
#include <iterator>
#include <vector>
#include <map>
#include <set>
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
	#include <dirent.h>
#endif

#if RN_PLATFORM_MAC_OS
	#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED 1

	#include <OpenGL/gl3.h>
	#include <OpenGL/gl3ext.h>

	#include <CoreGraphics/CoreGraphics.h>
#endif

#if RN_PLATFORM_IOS
	#include <OpenGLES/ES2/gl.h>
	#include <OpenGLES/ES2/glext.h>

	#include <libkern/OSAtomic.h>
	#include "glext.h"
#endif

#if RN_PLATFORM_WINDOWS
	#define WINDOWS_LEAN_AND_MEAN
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
#include "RNLockGuard.h"

// ---------------------------
// Helper macros
// ---------------------------

namespace RN
{
#if RN_PLATFORM_POSIX
	#define RN_EXPECT_TRUE(x)  __builtin_expect(!!(x), 1)
	#define RN_EXPECT_FALSE(x) __builtin_expect(!!(x), 0)
#endif
	
#if RN_PLATFORM_WINDOWS
	#define RN_EXPECT_TRUE(x)  (x)
	#define RN_EXPECT_FALSE(x) (x)
#endif
	
#ifndef NDEBUG
	#define RN_ASSERT(e, ...) RN_EXPECT_FALSE(!(e)) ? RN::__Assert(__PRETTY_FUNCTION__, __FILE__, __LINE__, #e, __VA_ARGS__) : (void)0
#else
	#define RN_ASSERT(e, ...) (void)0
#endif
	
	RNAPI RN_NORETURN void __Assert(const char *func, const char *file, int line, const char *expression, const char *message, ...);
	
	RNAPI RN_NORETURN void __HandleException(const Exception& e);
	RNAPI void ParseCommandLine(int argc, char *argv[]);
	
	RNAPI uint32 ABIVersion();
	RNAPI uint32 Version();
	RNAPI uint32 VersionMake(uint32 major, uint32 minor, uint32 patch);
	
	RNAPI uint32 VersionMajor();
	RNAPI uint32 VersionMinor();
	RNAPI uint32 VersionPatch();
	
	typedef uint32 FrameID;
	
	enum class ComparisonResult : int
	{
		LessThan   = -1,
		EqualTo     = 0,
		GreaterThan = 1
	};
	
	class Range
	{
	public:
		Range() {}
		Range(size_t torigin, size_t tlength)
		{
			origin = torigin;
			length = tlength;
		}
		
		size_t GetEnd() const
		{
			return origin + length;
		}
		
		bool Contains(const Range& other) const
		{
			return (other.origin >= origin && GetEnd() <= other.GetEnd());
		}
		
		bool Overlaps(const Range& other) const
		{
			return (GetEnd() >= other.origin && origin <= other.GetEnd());
		}
		
		size_t origin;
		size_t length;
	};
	
	template <typename T>
	class Singleton
	{
	public:
		static T *GetSharedInstance()
		{
			LockGuard<SpinLock> lock(_lock);
			
			if(!_instance)
				_instance = new T();

			return _instance;
		}

	protected:
		Singleton()
		{}

		virtual ~Singleton()
		{
			LockGuard<SpinLock> lock(_lock);
			
			if(_instance == this)
				_instance = nullptr;
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
		static T *GetSharedInstance()
		{
			return _instance;
		}

	protected:
		NonConstructingSingleton()
		{
			RN_ASSERT(_instance == 0, "");
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
		
		
	template <class T>
	class PIMPL
	{
	public:
		PIMPL() :
			_ptr(new T)
		{}
		
		PIMPL(T *val) :
			_ptr(val)
		{}
		
		operator T* ()
		{
			return _ptr.get();
		}
		operator const T* () const
		{
			return _ptr.get();
		}
		
		T *operator ->()
		{
			return _ptr.get();
		}
		const T *operator ->() const
		{
			return _ptr.get();
		}
		
	private:
		std::unique_ptr<T> _ptr;
	};
}

#endif /* __RAYNE_BASE_H__ */
