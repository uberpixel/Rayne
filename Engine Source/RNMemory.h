//
//  RNMemory.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MEMORY_H__
#define __RAYNE_MEMORY_H__

#include "RNBase.h"
#include "RNSIMD.h"

namespace RN
{
	namespace Memory
	{
		RNAPI void *AllocateAligned(size_t size, size_t alignment);
		RNAPI void FreeAligned(void *ptr);

		RNAPI void *AllocateSIMD(size_t size);
		RNAPI void FreeSIMD(void *ptr);
	};
}

#endif /* __RAYNE_MEMORY_H__ */
