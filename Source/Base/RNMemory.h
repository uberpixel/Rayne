//
//  RNMemory.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MEMORY_H__
#define __RAYNE_MEMORY_H__

#ifdef RN_BUILD_LIBRARY
	#include <RayneConfig.h>
#else
	#include "../RayneConfig.h"
#endif

#include <new>

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
		RNAPI void *Allocate(size_t size, const std::nothrow_t &n) RN_NOEXCEPT;
		RNAPI void *AllocateArray(size_t size, const std::nothrow_t &n) RN_NOEXCEPT;
		RNAPI void Free(void *ptr) RN_NOEXCEPT;
		RNAPI void FreeArray(void *ptr) RN_NOEXCEPT;
		RNAPI void Free(void *ptr, const std::nothrow_t &n) RN_NOEXCEPT;
		RNAPI void FreeArray(void *ptr, const std::nothrow_t &n) RN_NOEXCEPT;
		
		class PoolAllocator;
		class Pool
		{
		public:
			Pool(size_t alignment = 8);
			~Pool();
			
			void *Allocate(size_t size);
			void *Allocate(size_t size, const std::nothrow_t &n) RN_NOEXCEPT;
			
			void Evict(bool willReuse = false);
			
		private:
			PoolAllocator *_allocator;
		};
	};
}

#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
void *operator new(size_t size);
void *operator new[](size_t size);
void *operator new(size_t size, const std::nothrow_t &n) RN_NOEXCEPT;
void *operator new[](size_t size, const std::nothrow_t &n) RN_NOEXCEPT;

void operator delete(void *ptr) RN_NOEXCEPT;
void operator delete[](void *ptr) RN_NOEXCEPT;
void operator delete(void *ptr, const std::nothrow_t &n) RN_NOEXCEPT;
void operator delete[](void *ptr, const std::nothrow_t &n) RN_NOEXCEPT;
#endif

#endif /* __RAYNE_MEMORY_H__ */
