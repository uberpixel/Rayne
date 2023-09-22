//
//  RNMemory.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMemory.h"
#include "RNBase.h"

namespace RN
{
	namespace Memory
	{
		void *AllocateAligned(size_t size, size_t alignment)
		{
#if RN_PLATFORM_POSIX
			if(alignment < sizeof(void *))
				alignment = sizeof(void *);

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
		
		void *Allocate(size_t size)
		{
			return malloc(size);
		}
		void *AllocateArray(size_t size)
		{
			return malloc(size);
		}
		void *Allocate(size_t size, const std::nothrow_t &n) RN_NOEXCEPT
		{
			return malloc(size);
		}
		void *AllocateArray(size_t size, const std::nothrow_t &n) RN_NOEXCEPT
		{
			return malloc(size);
		}
		
		
		void Free(void *ptr) RN_NOEXCEPT
		{
			free(ptr);
		}
		void FreeArray(void *ptr) RN_NOEXCEPT
		{
			free(ptr);
		}
		void Free(void *ptr, const std::nothrow_t &n) RN_NOEXCEPT
		{
			free(ptr);
		}
		void FreeArray(void *ptr, const std::nothrow_t &n) RN_NOEXCEPT
		{
			free(ptr);
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
			
			void *Allocate(size_t size, const std::nothrow_t &n) RN_NOEXCEPT
			{
				return AllocateFromBucket(size);
			}
			
			void Evict(bool reuse)
			{
				_lastSize = std::max(static_cast<size_t>(2048), _lastSize / 2);
				_fullBuckets.clear();
				
				// Move half of the full buckets into the free list
				if(_fullBuckets.size() >= 2)
				{
					auto i   = _fullBuckets.begin();
					auto end = reuse ? _fullBuckets.end() : i;
					
					if(!reuse)
						std::advance(end, _fullBuckets.size() / 2);
					
					for(; i != end; i ++)
						_freeBuckets.push_back(std::move(*i));
				}
				
				for(auto &bucket : _freeBuckets)
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
				
				AllocationBucket(AllocationBucket &&other)
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
				
				AllocationBucket &operator= (AllocationBucket &&other)
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
			
			AllocationBucket &CreateBucket(size_t minSize)
			{
				_lastSize = std::max(minSize, _lastSize * 2);
				_freeBuckets.emplace_back(AllocationBucket(_lastSize));
				
				return _freeBuckets.back();
			}
			
			void *AllocateFromBucket(size_t size)
			{
				for(auto i = _freeBuckets.begin(); i != _freeBuckets.end();)
				{
					auto &bucket = *i;
					
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
				
				AllocationBucket &bucket = CreateBucket(size);
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
		
		void *Pool::Allocate(size_t size, const std::nothrow_t &n) RN_NOEXCEPT
		{
			return _allocator->Allocate(size, n);
		}
		
		void Pool::Evict(bool willReuse)
		{
			_allocator->Evict(willReuse);
		}
	};
}

//Overwrite new and delete here to use custom memory allocator for everything, but default ones are pretty good already.
/*
void *operator new(size_t size)
{
	return RN::Memory::Allocate(size);
}
void *operator new[](size_t size)
{
	return RN::Memory::AllocateArray(size);
}
void *operator new(size_t size, const std::nothrow_t &n) RN_NOEXCEPT
{
	return RN::Memory::Allocate(size, n);
}
void *operator new[](size_t size, const std::nothrow_t &n) RN_NOEXCEPT
{
	return RN::Memory::AllocateArray(size, n);
}


void operator delete(void *ptr) RN_NOEXCEPT
{
	return RN::Memory::Free(ptr);
}
void operator delete[](void *ptr) RN_NOEXCEPT
{
	return RN::Memory::FreeArray(ptr);
}
void operator delete(void *ptr, const std::nothrow_t &n) RN_NOEXCEPT
{
	return RN::Memory::Free(ptr, n);
}
void operator delete[](void *ptr, const std::nothrow_t &n) RN_NOEXCEPT
{
	return RN::Memory::FreeArray(ptr, n);
}
*/
