//
//  RNException.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_EXCEPTION_H__
#define __RAYNE_EXCEPTION_H__

#include <string>
#include <vector>

#ifdef RN_BUILD_LIBRARY
	#include <RayneConfig.h>
#else
	#include "../RayneConfig.h"
#endif

namespace RN
{
	class Thread;
	class String;
	class Exception : public std::exception
	{
	public:
		RNAPI Exception(const std::string &reason);
		RNAPI Exception(const String *reason);

		Thread *GetThread() const { return _thread; }
		const std::string &GetReason() const { return _reason; }
		const std::vector<std::pair<uintptr_t, std::string>> &GetCallStack() const { return _callStack; }

		RNAPI const char *what() const RN_NOEXCEPT override;

	private:
		void GatherInfo();

		std::string _reason;
		
		Thread *_thread;
		std::vector<std::pair<uintptr_t, std::string>> _callStack;
	};

	#define RNExceptionType(name) \
		class name##Exception : public RN::Exception \
		{ \
		public: \
			RNAPI name##Exception(const std::string &reason); \
			RNAPI name##Exception(const String *reason); \
		};

	#define RNExceptionImp(name) \
		name##Exception::name##Exception(const std::string &reason) : \
			RN::Exception(reason) \
		{} \
		name##Exception::name##Exception(const String *reason) : \
			RN::Exception(reason) \
		{}

	RNExceptionType(InvalidArgument)
	RNExceptionType(Range)
	RNExceptionType(Downcast)
	RNExceptionType(Inconsistency)
	RNExceptionType(InvalidCall)
	RNExceptionType(NotImplemented)
}

#endif /* __RAYNE_EXCEPTION_H__ */
