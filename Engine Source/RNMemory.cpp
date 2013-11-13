//
//  RNMemory.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <tcmalloc.h>
#include "RNMemory.h"
#include "RNSIMD.h"

#if RN_SIMD
	#if __SSE__
		#define RN_SIMD_ALIGNMENT 16
	#endif
#else
	#define RN_SIMD_ALIGNMENT 0
#endif

namespace RN
{
	namespace Memory
	{
		void *AllocateAligned(size_t size, size_t alignment)
		{
			void *ptr;
			int result = tc_posix_memalign(&ptr, alignment, size);
			
			return (result == 0) ? ptr : 0;
		}
		void FreeAligned(void *ptr)
		{
			tc_free(ptr);
		}
		
		
		void *AllocateSIMD(size_t size)
		{
#if RN_SIMD
			return AllocateAligned(size, RN_SIMD_ALIGNMENT);
#else
			return tc_malloc(size);
#endif
		}
		void FreeSIMD(void *ptr)
		{
#if RN_SIMD
			FreeAligned(ptr);
#else
			tc_free(ptr);
#endif
		}
		
		
		void *Allocate(size_t size)
		{
			return tc_new(size);
		}
		void *AllocateArray(size_t size)
		{
			return tc_newarray(size);
		}
		void *Allocate(size_t size, const std::nothrow_t& n) noexcept
		{
			return tc_new_nothrow(size, n);
		}
		void *AllocateArray(size_t size, const std::nothrow_t& n) noexcept
		{
			return tc_newarray_nothrow(size, n);
		}
		
		
		void Free(void *ptr) noexcept
		{
			tc_delete(ptr);
		}
		void FreeArray(void *ptr) noexcept
		{
			tc_deletearray(ptr);
		}
		void Free(void *ptr, const std::nothrow_t& n) noexcept
		{
			tc_delete_nothrow(ptr, n);
		}
		void FreeArray(void *ptr, const std::nothrow_t& n) noexcept
		{
			tc_deletearray_nothrow(ptr, n);
		}
	};
}

void *operator new(size_t size)
{
	return RN::Memory::Allocate(size);
}
void *operator new[](size_t size)
{
	return RN::Memory::AllocateArray(size);
}
void *operator new(size_t size, const std::nothrow_t& n) noexcept
{
	return RN::Memory::Allocate(size, n);
}
void *operator new[](size_t size, const std::nothrow_t& n) noexcept
{
	return RN::Memory::AllocateArray(size, n);
}


void operator delete(void *ptr) noexcept
{
	return RN::Memory::Free(ptr);
}
void operator delete[](void *ptr) noexcept
{
	return RN::Memory::FreeArray(ptr);
}
void operator delete(void *ptr, const std::nothrow_t& n) noexcept
{
	return RN::Memory::Free(ptr, n);
}
void operator delete[](void *ptr, const std::nothrow_t& n) noexcept
{
	return RN::Memory::FreeArray(ptr, n);
}
