//
//  RNBase.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef _RAYNE_BASE_H_
#define _RAYNE_BASE_H_

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
#include <array>
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
#include "RNException.h"
#include "RNOptions.h"
#include "RNExpected.h"
#include "RNSingleton.h"
#include "RNScopeGuard.h"
#include "RNLockGuard.h"
#include "../Math/RNConstants.h"
#include "../Math/RNMath.h"
#include "../Threads/RNSpinLock.h"

// ---------------------------
// Platform dependent includes
// ---------------------------

#if RN_PLATFORM_MAC_OS
	#include <mach/mach.h>
#endif

// ---------------------------
// Helper macros
// ---------------------------

#define kRNVersionMajor 0
#define kRNVersionMinor 7
#define kRNVersionPatch 0

#define kRNABIVersion 10

#define RN_ASSERT(e, ...) RN_EXPECT_FALSE(!(e)) ? RN::__Assert(RN_FUNCTION_SIGNATURE, __FILE__, __LINE__, #e, __VA_ARGS__) : (void)0

#define RN_REGISTER_INITIALIZER(name, body) \
	namespace { \
		static void __RNGlobalInit##name##Callback() { body; } \
		static RN::Initializer __RNGlobalInit##name (__RNGlobalInit##name##Callback, nullptr); \
	}

#define RN_REGISTER_DESTRUCTOR(name, body) \
	namespace { \
		static void __RNGlobalDestructor##name##Callback() { body; } \
		static RN::Initializer __RNGlobalDestructor##name (nullptr, __RNGlobalDestructor##name##Callback); \
	}

#if RN_PLATFORM_POSIX
	#define RN_EXPECT_TRUE(x)  __builtin_expect(!!(x), 1)
	#define RN_EXPECT_FALSE(x) __builtin_expect(!!(x), 0)
#endif

#if RN_PLATFORM_WINDOWS
	#define RN_EXPECT_TRUE(x)  (x)
	#define RN_EXPECT_FALSE(x) (x)
#endif

namespace RN
{
	class Kernel;

	Kernel *Initialize(int argc, char *argv[]);

	RNAPI RN_NORETURN void __Assert(const char *func, const char *file, int line, const char *expression, const char *message, ...);

	typedef uint64 Tag;

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
		Range() = default;
		Range(size_t torigin, size_t tlength) :
				origin(torigin),
				length(tlength)
		{}

		size_t GetEnd() const
		{
			return origin + length;
		}

		bool Contains(const Range &other) const
		{
			return (other.origin >= origin && GetEnd() >= other.GetEnd());
		}

		bool Overlaps(const Range &other) const
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

#endif /* _RAYNE_BASE_H_ */
