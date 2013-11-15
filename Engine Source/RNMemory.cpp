//
//  RNMemory.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <tcmalloc.h>
#include "RNMemory.h"
#include "RNBase.h"
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
		
		
		class PoolAllocator
		{
		public:
			PoolAllocator(size_t alignment) :
				_alignment(alignment),
				_lastSize(2048)
			{
				CreateBucket(_lastSize);
			}
			
			~PoolAllocator()
			{}
			
			
			void *Allocate(size_t size)
			{
				return AllocateFromBucket(size);
			}
			
			void *Allocate(size_t size, const std::nothrow_t& n) noexcept
			{
				return AllocateFromBucket(size);
			}
			
			void Evict()
			{
				_lastSize = std::max(static_cast<size_t>(2048), _lastSize / 2);
				_fullBuckets.clear();
				
				// Move half of the full buckets into the free list
				if(_fullBuckets.size() > 2)
				{
					auto i = _fullBuckets.begin();
					auto end = i;
					
					std::advance(end, _fullBuckets.size() / 2);
					
					for(; i != end; i ++)
						_freeBuckets.push_back(std::move(*i));
				}
				
				for(auto& bucket : _freeBuckets)
					bucket.offset = 0;
			}
			
		private:
			struct AllocationBucket
			{
				AllocationBucket(size_t tsize)
				{
					size   = tsize;
					offset = 0;
					buffer = reinterpret_cast<uint8 *>(malloc(size));
				}
				
				AllocationBucket(AllocationBucket&& other)
				{
					size = other.size;
					offset = other.offset;
					buffer = other.buffer;
					
					other.buffer = nullptr;
					other.size   = 0;
				}
				
				~AllocationBucket()
				{
					if(buffer)
						free(buffer);
				}
				
				AllocationBucket& operator= (AllocationBucket&& other)
				{
					size = other.size;
					offset = other.offset;
					buffer = other.buffer;
					
					other.buffer = nullptr;
					other.size   = 0;
					
					return *this;
				}
				
				uint8 *buffer;
				size_t offset;
				size_t size;
			};
			
			AllocationBucket& CreateBucket(size_t minSize)
			{
				_lastSize = std::max(minSize, _lastSize * 2);
				_freeBuckets.emplace_back(AllocationBucket(_lastSize));
				
				return _freeBuckets.back();
			}
			
			void *AllocateFromBucket(size_t size)
			{
				for(auto i = _freeBuckets.begin(); i != _freeBuckets.end();)
				{
					auto& bucket = *i;
					
					if(bucket.offset + size <= bucket.size)
					{
						void *pointer = (bucket.buffer + bucket.offset);
						bucket.offset += size;
						bucket.offset += (bucket.offset % _alignment);
						
						return pointer;
					}
					else
					{
						AllocationBucket temp = std::move(bucket);
					
						i = _freeBuckets.erase(i);
						_fullBuckets.push_back(std::move(temp));
						
						continue;
					}
					
					i ++;
				}
				
				AllocationBucket& bucket = CreateBucket(size);
				bucket.offset += size;
				bucket.offset += (bucket.offset % _alignment);
				
				return reinterpret_cast<void *>(bucket.buffer);
			}
			
			size_t _alignment;
			size_t _lastSize;
			
			std::vector<AllocationBucket> _freeBuckets;
			std::vector<AllocationBucket> _fullBuckets;
		};
		
		
		Pool::Pool(size_t alignment) :
			_allocator(new PoolAllocator(alignment))
		{}
		
		Pool::~Pool()
		{
			delete _allocator;
		}
		
		void *Pool::Allocate(size_t size)
		{
			return _allocator->Allocate(size);
		}
		
		void *Pool::Allocate(size_t size, const std::nothrow_t& n) noexcept
		{
			return _allocator->Allocate(size, n);
		}
		
		void Pool::Evict()
		{
			_allocator->Evict();
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
