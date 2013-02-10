//
//  RNBase.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBase.h"
#include "RNBaseInternal.h"
#include "RNError.h"

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
}

#if RN_PLATFORM_MAC_OS

int main(int argc, char *argv[])
{
	int result = 0;
	
	try
	{
		result = NSApplicationMain(argc, (const char **)argv);
	}
	catch(RN::ErrorException e)
	{
		fprintf(stderr, "Caught exception %i|%i|%i.\nReason: %s\n", e.Group(), e.Subgroup(), e.Code(), e.Description().c_str());
		fflush(stderr);
		
		if(e.Description().length() > 0)
		{
			[[NSAlert alertWithMessageText:@"Rayne crashed" defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:@"Reason: %s\nException code: %i|%i|%i", e.Description().c_str(), e.Group(), e.Subgroup(), e.Code()] runModal];
			[NSApp terminate:nil];
		}
	}
	
	return result;
}

#endif

#if RN_PLATFORM_IOS

int main(int argc, char *argv[])
{
	int result = 0;
	
	try
	{
		@autoreleasepool {
			result = UIApplicationMain(argc, argv, nil, @"RNAppDelegate");
		}
	}
	catch(RN::ErrorException e)
	{
		fprintf(stderr, "Caught exception %i|%i|%i.\nReason: %s\n", e.Group(), e.Subgroup(), e.Code(), e.Description().c_str());
		fflush(stderr);
	}
	
	return result;
}

#endif
