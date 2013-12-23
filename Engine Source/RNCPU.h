//
//  RNCPU.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CPU_H__
#define __RAYNE_CPU_H__

#include "RNBase.h"

namespace RN
{
#if RN_PLATFORM_INTEL
	namespace X86_64
	{
		enum class Vendors
		{
			Unknown,
			Intel,
			AMD
		};
		enum
		{
			CAP_SSE   = (1 << 0),
			CAP_SSE2  = (1 << 1),
			CAP_SSE3  = (1 << 2),
			CAP_SSSE3 = (1 << 3),
			CAP_SSE41 = (1 << 4),
			CAP_SSE42 = (1 << 5),
			CAP_AVX   = (1 << 6),
			
			CAP_HT     = (1 << 7),
			CAP_RDRAND = (1 << 8)
		};
		typedef uint32 Capabilities;
		
		struct CPUIDRegisters
		{
			uint32 eax;
			uint32 ebx;
			uint32 ecx;
			uint32 edx;
		};
		
		Vendors GetVendor();
		Capabilities GetCapabilites();
		
		void CPUID(CPUIDRegisters *registers);
	}
#endif
}

#endif /* __RAYNE_CPU_H__ */
