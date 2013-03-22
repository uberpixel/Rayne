//
//  RNASM.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ASM_H__
#define __RAYNE_ASM_H__

#include "RNPlatform.h"

#define FALIGN 4,0x90

#define EXT(x) _##x
#define LEXT(x) _##x:

#define	ENTRY(x) .global EXT(x); .align FALIGN;	LEXT(x)
#define EXTERN(x) .extern EXT(x)

#endif

// Because I'm a fucknut and tend to forgot this:
// 64 bit | 32 bit | 16 bit | 8 bit
// RAX      EAX      AX       AL
// RBX      EBX      BX       BL
// RCX      ECX      CX       CL
// RDX      EDX      DX       DL
// RBP      EBP      BP
// RSI      ESI      SI
// RDI      EDI      DI
// ESP      ESP      SP
// 

// x86 calling convention (aka cdecl)
// 1) EAX, ECX and EDX are caller saved
// 2) Arguments are pushed in reversed order on the stack
// 3) Stack cleanup is done by the caller

// x86-64 calling convention for POSIX systems (aka System V AMD64 ABI)
// 1) EAX, ECX and EDX are caller saved
// 2) Arguments are passed in the RDI, RSI, RDX, RCX, R8, R9 (for integers and pointers), XMM0 to XMM7 registers (for floatings), then in reversed order on the stack
// 3) The stack is aligned on a 16byte boundary
// 4) Stack cleanup is done by the caller

// x86-64 calling convention for Windows
// 1) EAX, ECX and EDX are caller saved
// 2) Arguments are passed in the RCX, RDX, R8 and R9 registers (for integers or pointers), XMM0, XMM1, XMM2, XMM3 registers (for floatings), then in reversed order on the stack
// 3) There is a 32byte shadow space on the stack between the return address and the first pushed argument
// 4) The stack is aligned on a 16byte boundary
// 5) Stack cleanup is done by the caller
