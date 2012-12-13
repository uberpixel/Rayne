//
//  RNBase.cpp
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBase.h"

namespace RN
{
	void Assert(bool condition, const char *message)
	{
		if(!condition)
		{
			#if RN_PLATFORM_POSIX
				raise(SIGTRAP);
			#endif
			
			#if RN_PLATFORM_WINDOWS
				__debugbreak();
			#endif
		}
	}
}
