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

#if RN_TARGET_CXX_NOXCEPT
	#define RN_NOEXCEPT noexcept
#else
	#define RN_NOEXCEPT
#endif

namespace RN
{
	namespace Memory
	{
		void *AllocateAligned(size_t size, size_t alignment);
		void FreeAligned(void *ptr);

		void *AllocateSIMD(size_t size);
		void FreeSIMD(void *ptr);
		
		void *Allocate(size_t size);
		void *AllocateArray(size_t size);
		void *Allocate(size_t size, const std::nothrow_t& n) RN_NOEXCEPT;
		void *AllocateArray(size_t size, const std::nothrow_t& n) RN_NOEXCEPT;
		void Free(void *ptr) RN_NOEXCEPT;
		void FreeArray(void *ptr) RN_NOEXCEPT;
		void Free(void *ptr, const std::nothrow_t& n) RN_NOEXCEPT;
		void FreeArray(void *ptr, const std::nothrow_t& n) RN_NOEXCEPT;
		
		class PoolAllocator;
		class Pool
		{
		public:
			Pool(size_t alignment = 8);
			~Pool();
			
			void *Allocate(size_t size);
			void *Allocate(size_t size, const std::nothrow_t& n) RN_NOEXCEPT;
			
			void Evict(bool willReuse = false);
			
		private:
			PoolAllocator *_allocator;
		};
	};
}

void *operator new(size_t size);
void *operator new[](size_t size);
void *operator new(size_t size, const std::nothrow_t& n) RN_NOEXCEPT;
void *operator new[](size_t size, const std::nothrow_t& n) RN_NOEXCEPT;

void operator delete(void *ptr) RN_NOEXCEPT;
void operator delete[](void *ptr) RN_NOEXCEPT;
void operator delete(void *ptr, const std::nothrow_t& n) RN_NOEXCEPT;
void operator delete[](void *ptr, const std::nothrow_t& n) RN_NOEXCEPT;

#endif /* __RAYNE_MEMORY_H__ */
