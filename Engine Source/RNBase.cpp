//
//  RNBase.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <execinfo.h>
#include "RNBase.h"
#include "RNBaseInternal.h"
#include "RNError.h"
#include "RNThread.h"

namespace RN
{
	void __Assert(const char *func, int line, const char *expression, const char *message, ...)
	{
		fprintf(stderr, "%s(), assertion '%s' failed!\n", func, expression);
		
		if(message)
		{
			va_list args;
			va_start(args, message);
			
			fprintf(stderr, "Reason: \"");
			vfprintf(stderr, message, args);
			fprintf(stderr, "\"\n");
			
			va_end(args);
		}
		
		abort();
	}
	
	void __HandleException(const ErrorException& e)
	{
		const std::vector<std::pair<uintptr_t, std::string>>& callstack = e.CallStack();
		
		fprintf(stderr, "Caught exception %i|%i|%i.\nReason: %s\nDetails: %s\n", e.Group(), e.Subgroup(), e.Code(), e.Description().c_str(), e.AdditionalDetails().c_str());
		fprintf(stderr, "Chrashing Thread: %s\nBacktrace:\n", e.Thread()->Name().c_str());
		
		for(auto i=callstack.begin(); i!=callstack.end(); i++)
		{
			std::pair<uintptr_t, std::string> frame = *i;
			fprintf(stderr, "   0x%8lx %s\n", frame.first, frame.second.c_str());
		}
		
		fflush(stderr);
		abort();
	}		
}
