//
//  RNSpinLock.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SPINLOCK_H__
#define __RAYNE_SPINLOCK_H__

#include "RNBase.h"

namespace RN
{
	class SpinLock
	{
	public:
		SpinLock()
		{
			_lock = 0;
		}
		
		void Lock()
		{
#if RN_PLATFORM_INTEL
			uintptr_t address = (uintptr_t)&_lock;
	
#if RN_PLATFORM_64BIT
			__asm__ volatile("movq $0x1, %%rcx \n"
							 "movq %0, %%rbx \n"
							 "SpinlockWait: \n"
							 "xorq %%rax, %%rax \n"
							 "lock cmpxchgq %%rcx, (%%rbx) \n"
							 "jne SpinlockWait" : : "m" (address) : "%rcx", "%rax", "%rbx");
#endif
#if RN_PLATFORM_32BIT
			__asm__ volatile("movl $0x1, %%ecx \n"
							 "movl %0, %%ebx \n"
							 "SpinlockWait: \n"
							 "xorl %%eax, %%eax \n"
							 "lock cmpxchgl %%ecx, (%%ebx) \n"
							 "jne SpinlockWait" : : "m" (address) : "%ecx", "%eax", "%ebx");
#endif
#endif
		}
		
		void Unlock()
		{
			_lock = 0;
		}
		
	private:
		uint32 _lock;
	};
}

#endif /* __RAYNE_SPINLOCK_H__ */
