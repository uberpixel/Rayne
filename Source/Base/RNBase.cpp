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
	struct __KernelBootstrapHelper
	{
	public:
		static Kernel *BootstrapKernel()
		{
			Kernel *result = new Kernel();
			result->Bootstrap();

			return result;
		}
	};

	Kernel *Initialize(int argc, char *argv[])
	{
		Kernel *result = __KernelBootstrapHelper::BootstrapKernel();
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

	uint32 GetABIVersion() RN_NOEXCEPT
	{
		return kRNABIVersion;
	}
	uint32 GetAPIVersion() RN_NOEXCEPT
	{
		return static_cast<uint32>((kRNVersionMajor << 16) | (kRNVersionMinor << 8) | (kRNVersionPatch));
	}

	uint32 GetMajorVersion() RN_NOEXCEPT
	{
		return kRNVersionMajor;
	}
	uint32 GetMinorVersion() RN_NOEXCEPT
	{
		return kRNVersionMinor;
	}
	uint32 GetPatchVersion() RN_NOEXCEPT
	{
		return kRNVersionPatch;
	}
}
