//
//  RNSpinLockInternal.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSpinLock.h"

extern "C"
{
	void RNPrimitiveSpinLockYield()
	{
		std::this_thread::yield();
	}
}
