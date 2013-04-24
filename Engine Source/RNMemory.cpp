//
//  RNMemory.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMemory.h"

namespace RN
{
	namespace Memory
	{
		void *AllocateAligned(size_t size, size_t alignment)
		{
#if RN_PLATFORM_POSIX
			void *ptr;
			int result = posix_memalign(&ptr, alignment, size);
			
			return (result == 0) ? ptr : 0;
#endif
			
#if RN_PLATFORM_WINDOWS
			return _aligned_malloc(size, alignment);
#endif
		}
		
		void FreeAligned(void *ptr)
		{
#if RN_PLATFORM_POSIX
			free(ptr);
#endif
			
#if RN_PLATFORM_WINDOWS
			_aligned_free(ptr);
#endif
		}
		
		
		void *AllocateSIMD(size_t size)
		{
#if __SSE__
			return AllocateAligned(size, 16);
#else
			return malloc(size);
#endif
		}
		
		void FreeSIMD(void *ptr)
		{
#if __SSE__
			FreeAligned(ptr);
#else
			free(ptr);
#endif
		}
	};
}
