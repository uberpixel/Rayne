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
#include "RNThread.h"
#include "RNSpinLock.h"
#include "RNPathManager.h"

#define kRNVersionMajor 0
#define kRNVersionMinor 1
#define kRNVersionPatch 0

#define kRNABIVersion 0

namespace RN
{
	static SpinLock __DieLock;
	
	void __Assert(const char *func, const char *file, int line, const char *expression, const char *message, ...)
	{
		__DieLock.Lock();
		fprintf(stderr, "Assertion '%s' failed in %s, %s:%i\n", expression, func, file, line);
		__DieLock.Unlock();
		
		va_list args;
		va_start(args, message);
		
		char reason[1024];
		vsprintf(reason, message, args);
		
		va_end(args);
		
		throw Exception(Exception::Type::InconsistencyException, reason);
	}
	
	void __HandleException(const Exception& e)
	{
		__DieLock.Lock();
		
		const std::vector<std::pair<uintptr_t, std::string>>& callstack = e.GetCallStack();
		
		fprintf(stderr, "Caught exception %s, reason: %s\n", e.GetStringifiedType(), e.GetReason().c_str());
		fprintf(stderr, "Chrashing Thread: %s\nBacktrace:\n", e.GetThread()->GetName().c_str());
		
		for(auto i=callstack.begin(); i!=callstack.end(); i++)
		{
			std::pair<uintptr_t, std::string> frame = *i;
			fprintf(stderr, "   0x%8lx %s\n", frame.first, frame.second.c_str());
		}
		
		fflush(stderr);
		abort();
		__DieLock.Unlock();
	}
	
	void ParseCommandLine(int argc, char *argv[])
	{
		for(int i=0; i<argc; i++)
		{
			if(strcmp(argv[i], "-r") == 0 && i < argc - 1)
			{
				char *path = argv[++ i];
				PathManager::AddSearchPath(path);
			}
		}
	}
	
	
	uint32 ABIVersion()
	{
		return kRNABIVersion;
	}
	
	uint32 Version()
	{
		return VersionMake(kRNVersionMajor, kRNVersionMinor, kRNVersionPatch);
	}
	
	uint32 VersionMake(uint32 major, uint32 minor, uint32 patch)
	{
		return static_cast<uint32>((major << 16) | (minor << 8) | (patch));
	}
	
	
	uint32 VersionMajor()
	{
		return kRNVersionMajor;
	}
	
	uint32 VersionMinor()
	{
		return kRNVersionMinor;
	}
	
	uint32 VersionPatch()
	{
		return kRNVersionPatch;
	}
}
