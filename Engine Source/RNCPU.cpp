//
//  RNCPU.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCPU.h"

namespace RN
{
#if RN_PLATFORM_INTEL
	namespace X86_64
	{
		static Vendors CPUVendor = Vendors::Unknown;
		static Capabilities CPUCaps = 0;
		
		
		Vendors GetVendor()
		{
			return CPUVendor;
		}
		
		Capabilities GetCapabilites()
		{
			return CPUCaps;
		}
		
		
		void CPUID(CPUIDRegisters *registers)
		{
			int *regs = reinterpret_cast<int *>(registers);
			int code = static_cast<int>(registers->eax);
			
#if RN_PLATFORM_POSIX
			__asm__ volatile("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "a" (code), "c" (0));
#endif
		}
		
		
		void ReadCPUVendor()
		{
			CPUIDRegisters regs = { 0, 0, 0, 0 };
			CPUID(&regs);
			
			char vendor[13];
			memcpy(vendor + 0, &regs.ebx, 4);
			memcpy(vendor + 4, &regs.edx, 4);
			memcpy(vendor + 8, &regs.ecx, 4);
			
			vendor[12] = '\0';
			
			if(strcmp(vendor, "AuthenticAMD") == 0)
			{
				CPUVendor = Vendors::AMD;
			}
			if(strcmp(vendor, "GenuineIntel") == 0)
			{
				CPUVendor = Vendors::Intel;
			}
		}
		
		void ReadCPUCaps()
		{
			CPUIDRegisters regs = { 1, 0, 0, 0 };
			CPUID(&regs);
			
			CPUCaps |= (regs.ecx & (1 << 0)) ? CAP_SSE3 : 0;
			CPUCaps |= (regs.ecx & (1 << 9)) ? CAP_SSSE3 : 0;
			CPUCaps |= (regs.ecx & (1 << 19)) ? CAP_SSE41 : 0;
			CPUCaps |= (regs.ecx & (1 << 20)) ? CAP_SSE42 : 0;
			CPUCaps |= (regs.ecx & (1 << 30)) ? CAP_RDRAND : 0;
			
			CPUCaps |= (regs.edx & (1 << 25)) ? CAP_SSE : 0;
			CPUCaps |= (regs.edx & (1 << 26)) ? CAP_SSE2 : 0;
			CPUCaps |= (regs.edx & (1 << 28)) ? CAP_HT : 0; // TODO: AMD doesn't implement HT, but claims to... What about that?
		}
		
		
		void GetCPUInfo()
		{
			ReadCPUVendor();
			ReadCPUCaps();
		}
	}
#endif
}
