//
//  RNBumpAllocator.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RN_BUMP_ALLOCATOR_H__
#define __RN_BUMP_ALLOCATOR_H__

#include "RNBase.h"

namespace RN
{
	class BumpAllocator
	{
	public:
		RNAPI BumpAllocator(size_t size);
		RNAPI ~BumpAllocator();

		RNAPI void *Alloc(size_t size);
		RNAPI void PutBack(size_t size);
		RNAPI void Reset();

		RNAPI static BumpAllocator *GetThreadAllocator();

		size_t GetCurrentSize() const noexcept { return _currentSize; }
		size_t GetMaxSize() const noexcept { return _maxSize; }

	private:
		struct Allocation
		{
			bool operator < (const Allocation &other) const { return size < other.size; }

			uint8 *memory;
			size_t size;
			size_t offset;
		};

		void AddAllocation(size_t size);

		std::vector<Allocation> _allocationStack;
		size_t _activeSlice;
		size_t _currentSize;
		size_t _maxSize;
	};
}

#endif /* __RN_BUMP_ALLOCATOR_H__ */
