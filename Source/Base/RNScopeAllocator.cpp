//
//  RNScopeAllocator.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNScopeAllocator.h"
#include "../Math/RNAlgorithm.h"
#include "../Threads/RNThreadLocalStorage.h"

namespace RN
{
#if RN_COMPILER_MSVC
#pragma pack(push)
#pragma pack(1)
#endif
	struct __ScopeAllocatorFinalizer
	{
		void(*destructor)(void *ptr);
		__ScopeAllocatorFinalizer *next;
		uint16 size;
		uint16 offset;
#if RN_COMPILER_GCC_LIKE
	} __attribute__((packed));
#else
	};
#pragma pack(pop)
#endif

	static ThreadLocalStorage<ScopeAllocator *> __topAllocator;

	ScopeAllocator::ScopeAllocator() :
		ScopeAllocator(GetThreadAllocator()._allocator)
	{}

	ScopeAllocator::ScopeAllocator(const ScopeAllocator &other) :
		ScopeAllocator(other._allocator)
	{}

	ScopeAllocator::ScopeAllocator(BumpAllocator &allocator) :
		_allocator(allocator),
		_finalizerChain(nullptr),
		_nonFinalizerSize(0)
	{
		_previous = &GetThreadAllocator();
		__topAllocator.SetValue(this);
	}
	ScopeAllocator::~ScopeAllocator()
	{
		size_t putBack = _nonFinalizerSize;

		__ScopeAllocatorFinalizer *finalizer = _finalizerChain;
		while(finalizer)
		{
			if(finalizer->destructor)
			{
				uint8 *buffer = reinterpret_cast<uint8 *>(finalizer) + finalizer->offset;
				finalizer->destructor(buffer);
			}

			putBack += finalizer->size;
			finalizer = finalizer->next;
		}

		if(putBack > 0)
			_allocator.PutBack(putBack);

		RN_ASSERT(__topAllocator.GetValue() == this, "Something broke the allocator chain!");
		__topAllocator.SetValue(_previous);
	}

	ScopeAllocator &ScopeAllocator::GetThreadAllocator()
	{
		return *__topAllocator.GetValue();
	}

	void *ScopeAllocator::AllocWithDestructor(size_t size, size_t alignment, void (*fn)(void *ptr))
	{
		size_t total = size + alignment + sizeof(__ScopeAllocatorFinalizer);

		__ScopeAllocatorFinalizer *finalizer = (__ScopeAllocatorFinalizer *)_allocator.Alloc(total);
		uint8 *base = reinterpret_cast<uint8 *>(finalizer);
		uint8 *buffer = base + sizeof(__ScopeAllocatorFinalizer);

		buffer += (alignment - (reinterpret_cast<uintptr_t>(buffer) % alignment)) % alignment;

		finalizer->destructor = fn;
		finalizer->next = _finalizerChain;
		finalizer->size = static_cast<uint16>(total);
		finalizer->offset = static_cast<uint16>(buffer - base);

		_finalizerChain = finalizer;

		return buffer;
	}
	void *ScopeAllocator::Alloc(size_t size, size_t alignment)
	{
		size_t total = size + alignment;
		uintptr_t buffer = (uintptr_t)_allocator.Alloc(total);

		// Round up to match alignment
		uintptr_t aligned = AlignUp(buffer, alignment);
		size_t difference = aligned - buffer;


		if(difference > 0 && difference < alignment)
		{
			size_t extra = alignment - difference;

			_allocator.PutBack(extra);
			total -= extra;
		}

		_nonFinalizerSize += total;

		return reinterpret_cast<void *>(aligned);
	}
}
