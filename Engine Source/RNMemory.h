//
//  RNMemory.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MEMORY_H__
#define __RAYNE_MEMORY_H__

#include <new>
#include "RNDefines.h"

namespace RN
{
	namespace Memory
	{
		RNAPI void *AllocateAligned(size_t size, size_t alignment);
		RNAPI void FreeAligned(void *ptr);

		RNAPI void *AllocateSIMD(size_t size);
		RNAPI void FreeSIMD(void *ptr);
		
		RNAPI void *Allocate(size_t size);
		RNAPI void *AllocateArray(size_t size);
		RNAPI void *Allocate(size_t size, const std::nothrow_t& n) noexcept;
		RNAPI void *AllocateArray(size_t size, const std::nothrow_t& n) noexcept;
		RNAPI void Free(void *ptr) noexcept;
		RNAPI void FreeArray(void *ptr) noexcept;
		RNAPI void Free(void *ptr, const std::nothrow_t& n) noexcept;
		RNAPI void FreeArray(void *ptr, const std::nothrow_t& n) noexcept;
		
		class PoolAllocator;
		class Pool
		{
		public:
			Pool(size_t alignment = 8);
			~Pool();
			
			void *Allocate(size_t size);
			void *Allocate(size_t size, const std::nothrow_t& n) noexcept;
			
			void Evict();
			
		private:
			PoolAllocator *_allocator;
		};
	};
}

RNAPI void *operator new(size_t size);
RNAPI void *operator new[](size_t size);
RNAPI void *operator new(size_t size, const std::nothrow_t& n) noexcept;
RNAPI void *operator new[](size_t size, const std::nothrow_t& n) noexcept;

RNAPI void operator delete(void *ptr) noexcept;
RNAPI void operator delete[](void *ptr) noexcept;
RNAPI void operator delete(void *ptr, const std::nothrow_t& n) noexcept;
RNAPI void operator delete[](void *ptr, const std::nothrow_t& n) noexcept;

#endif /* __RAYNE_MEMORY_H__ */
