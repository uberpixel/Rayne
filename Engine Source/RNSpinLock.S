//
//  RNSpinLock.S
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNASM.h"

EXTERN(RNPrimitiveSpinLockYield)

#if RN_PLATFORM_32BIT
// 32 bit version

ENTRY(RNPrimitiveSpinLockLock)
	pushl %edi
	movl 0x8(%esp), %edi
	movb $0x1, %cl

	jmp RNPrimitiveSpinLockTryObtain

RNPrimitiveSpinLockWait:
	// This is kind of stupid since RNPrimitiveSpinLockYield() is just a shim over std::this_thread::yield(), but we can't call it directly because of compiler optimizations
	// On OS X the call to yield() gets expanded to an invocation of callq _sched_yield after obtaining the correct mach port. Ugh.
	pushl %ecx
	call _RNPrimitiveSpinLockYield
	popl %ecx

RNPrimitiveSpinLockTryObtain:
	xorb %al, %al
	lock cmpxchgb %cl, (%edi)
	jne RNPrimitiveSpinLockWait

	popl %edi
	ret

ENTRY(RNPrimitiveSpinLockTryLock)
	pushl %edi
	movl 0x8(%esp), %edi
	movb $0x1, %cl
	xorb %al, %al
	lock cmpxchgb %cl, (%edi)
	jne RNPrimitiveSpinLockFailedObtain
	movl $0x1, %eax
	popl %edi
	ret

RNPrimitiveSpinLockFailedObtain:
	xorl %eax, %eax
	popl %edi
	ret

ENTRY(RNPrimitiveSpinLockUnlock)
	movl 0x4(%esp), %eax
	movb $0x0, (%eax)
	ret

#else
// 64 bit version

ENTRY(RNPrimitiveSpinLockLock)
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
	movb $0x1, %cl

	jmp RNPrimitiveSpinLockTryObtain

RNPrimitiveSpinLockWait:
	movq %rcx, -8(%rbp)
	movq %rdi, -16(%rbp)
	callq _RNPrimitiveSpinLockYield
	movq -8(%rbp), %rcx
	movq -16(%rbp), %rdi

RNPrimitiveSpinLockTryObtain:
	xorb %al, %al
	lock cmpxchgb %cl, (%rdi)
	jne RNPrimitiveSpinLockWait

	addq $32, %rsp
	popq %rbp
	ret

ENTRY(RNPrimitiveSpinLockTryLock)
	movb $0x1, %cl
	xorb %al, %al
	lock cmpxchgb %cl, (%rdi)
	jne RNPrimitiveSpinLockFailedObtain
	movq $0x1, %rax
	ret

RNPrimitiveSpinLockFailedObtain:
	xorq %rax, %rax
	ret

ENTRY(RNPrimitiveSpinLockUnlock)
	movq $0x0, (%rdi)
	ret

#endif
