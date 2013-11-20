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
#include "RNFileManager.h"
#include "RNLogging.h"

namespace RN
{
	void __Assert(const char *func, const char *file, int line, const char *expression, const char *message, ...)
	{
		va_list args;
		va_start(args, message);
		
		char reason[1024];
		vsprintf(reason, message, args);
		
		va_end(args);
		
		
		Log::Loggable loggable(Log::Level::Warning);
		
		loggable << "Assertion '" << expression << "' failed in " << func << ", " << file << ":" << line << std::endl;
		loggable << "Reason: " << reason;
		
		throw Exception(Exception::Type::InconsistencyException, reason);
	}
	
	void HandleException(const Exception& e)
	{
		{
			Log::Loggable loggable(Log::Level::Error);
			const std::vector<std::pair<uintptr_t, std::string>>& callstack = e.GetCallStack();
			
			loggable << "Man the lifeboats! Women and children first!" << std::endl;
			loggable << "Caught exception " << e.GetStringifiedType() << ", reason: " << e.GetReason() << std::endl;
			loggable << "Crashing thread: " << e.GetThread()->GetName() << std::endl << "Backtrace:" << std::endl;
			
			for(auto& frame : callstack)
			{
				loggable << "\t0x" << std::hex << frame.first << std::dec << " " << frame.second << std::endl;
			}
		}
		
		Log::Logger::GetSharedInstance()->Flush(true);
		
		delete Log::Logger::GetSharedInstance(); // Try to get a cleanly flushed log
		abort();
	}
	
	void ParseCommandLine(int argc, char *argv[])
	{
		for(int i = 0; i < argc; i ++)
		{
			if(strcmp(argv[i], "-r") == 0 && i < argc - 1)
			{
				char *path = argv[++ i];
				FileManager::GetSharedInstance()->AddSearchPath(path);
			}
		}
	}
	
	void Initialize(int argc, char *argv[])
	{
		ParseCommandLine(argc, argv);
		
#if RN_PLATFORM_MAC_OS
		@autoreleasepool
		{
			[NSApplication sharedApplication];
			[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
			
			[NSApp finishLaunching];
		}
#endif
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
