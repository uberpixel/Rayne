//
//  RNBase.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBase.h"
#include "RNKernel.h"

namespace RN
{
	Kernel *Initialize(int argc, char *argv[])
	{
		Kernel *result = new Kernel();

		return result;
	}

	void __Assert(const char *func, const char *file, int line, const char *expression, const char *message, ...)
	{
		va_list args;
		va_start(args, message);

		char reason[1024];
		vsprintf(reason, message, args);
		reason[1023] = '\0';

		va_end(args);


		/*{
			Log::Loggable loggable(Log::Level::Error);

			loggable << "Assertion '" << expression << "' failed in " << func << ", " << file << ":" << line << std::endl;
			loggable << "Reason: " << reason;
		}

		Log::Logger::GetSharedInstance()->Flush(true);

		delete Log::Logger::GetSharedInstance(); // Try to get a cleanly flushed log*/
		abort();
	}
}
