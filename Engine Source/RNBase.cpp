//
//  RNBase.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBase.h"

namespace RN
{
	void __Assert(const char *func, int line, const char *expression, const char *message, ...)
	{
		fprintf(stderr, "%s(), assertion '%s' failed!\n", func, expression);
		
		if(message)
		{
			va_list args;
			va_start(args, message);
			
			vfprintf(stderr, message, args);
			
			va_end(args);
		}			
		
		abort();
	}
}
