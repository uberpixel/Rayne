//
//  RNBase.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BASE_H__
#define __RAYNE_BASE_H__

#define NOMINMAX

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
#include <iomanip>
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

#include "RNEnum.h"
#include "RNDefines.h"
#include "RNConstants.h"
#include "RNException.h"
#include "RNOpenGL.h"

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
	#include <ShlObj.h>

	#undef near
	#undef far
#endif

#if RN_PLATFORM_LINUX

	#include <X11/extensions/Xrandr.h>
	#undef FilterNearest

	#include <GL/glx.h>
	#include "glext.h"
	
#endif

#include "RNMath.h"
#include "RNSIMD.h"
#include "RNSpinLock.h"
#include "RNLockGuard.h"
#include "RNSingleton.h"

// ---------------------------
// Helper macros
// ---------------------------

#define kRNVersionMajor 0
#define kRNVersionMinor 6
#define kRNVersionPatch 0

#define kRNABIVersion 4

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
	
#define RN_ASSERT(e, ...) RN_EXPECT_FALSE(!(e)) ? RN::__Assert(RN_FUNCTION_SIGNATURE, __FILE__, __LINE__, #e, __VA_ARGS__) : (void)0
	
#define RN_REGISTER_INIT(name, body) \
	namespace { \
		static void __RNGlobalInit##name##Callback() { body; } \
		static RN::Initializer __RNGlobalInit##name (__RNGlobalInit##name##Callback, nullptr); \
	}
	
#define RN_REGISTER_DESTRUCTOR(name, body) \
	namespace { \
		static void __RNGlobalDestructor##name##Callback() { body; } \
		static RN::Initializer __RNGlobalDestructor##name (__RNGlobalDestructor##name##Callback, nullptr); \
	}
	
	RNAPI RN_NORETURN void __Assert(const char *func, const char *file, int line, const char *expression, const char *message, ...);
	
	RNAPI RN_NORETURN void HandleException(const Exception& e);
	RNAPI void ParseCommandLine(int argc, char *argv[]);
	RNAPI void Initialize(int argc, char *argv[]);
	
	RNAPI uint32 GetABIVersion();
	RNAPI uint32 GetVersion();
	RNAPI uint32 GetVersionMajor();
	RNAPI uint32 GetVersionMinor();
	RNAPI uint32 GetVersionPatch();
	RNAPI bool IsDebugBuild();
	RNAPI uint32 VersionMake(uint32 major, uint32 minor, uint32 patch);
	
	typedef uint32 FrameID;
	typedef size_t Tag;
	
	enum class ComparisonResult : int
	{
		LessThan   = -1,
		EqualTo     = 0,
		GreaterThan = 1
	};
	
	class Initializer
	{
	public:
		typedef void (*Callback)();
		
		Initializer(Callback ctor, Callback dtor) :
			_dtor(dtor)
		{
			if(ctor)
				ctor();
		}
		
		~Initializer()
		{
			if(_dtor)
				_dtor();
		}
		
	private:
		Callback _dtor;
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
		
	template <class T>
	class PIMPL
	{
	public:
		PIMPL() :
			_ptr(new T)
		{}
		
		template<class ...Args>
		PIMPL(Args &&...args) :
			_ptr(new T(std::forward<Args>(args)...))
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
