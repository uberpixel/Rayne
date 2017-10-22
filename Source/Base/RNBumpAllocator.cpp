//
//  RNBumpAllocator.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBumpAllocator.h"
#include "../Math/RNAlgorithm.h"
#include "../Threads/RNThread.h"
#include "../Threads/RNThreadLocalStorage.h"

namespace RN
{
	static constexpr size_t kBumpAllocatorMultiplier = 4096;
	static ThreadLocalStorage<BumpAllocator *> __localAllocators;

	BumpAllocator &BumpAllocator::GetThreadAllocator()
	{
		BumpAllocator *allocator = __localAllocators.GetValue();
		if(!allocator)
		{
			allocator = new BumpAllocator(kBumpAllocatorMultiplier);
			__localAllocators.SetValue(allocator);

			Thread::GetCurrentThread()->ExecuteOnExit([allocator](void *unused) {
				delete allocator;
			}, allocator);
		}

		return *allocator;
	}

	BumpAllocator::BumpAllocator(size_t size) :
		_activeSlice(0),
		_currentSize(0),
		_maxSize(0)
	{
		AddAllocation(size);
	}
	BumpAllocator::~BumpAllocator()
	{
		RN_ASSERT(_activeSlice == 0, "BumpAllocator deleted with non-rolled back allocations");

		for(auto &alloc : _allocationStack)
		{
			RN_ASSERT(alloc.offset = 0, "BumpAllocator deleted with non-rolled back allocations");
			delete[] alloc.memory;
		}
	}

	void *BumpAllocator::Alloc(size_t size)
	{
		Allocation &alloc = _allocationStack[_activeSlice];

		if(alloc.offset + size >= alloc.size)
		{
			if(_activeSlice == _allocationStack.size() - 1)
				AddAllocation(std::max(size, static_cast<size_t>(_maxSize * 1.5)));
			else
				_activeSlice ++;

			return this->Alloc(size); // Recursively walk through the allocation stack... It can't be that deep, can it?
		}

		uint8_t *mem = alloc.memory + alloc.offset;
		alloc.offset += size;

		_currentSize += size;
		_maxSize = std::max(_currentSize, _maxSize);

		return mem;
	}
	void BumpAllocator::PutBack(size_t size)
	{
		RN_ASSERT(_currentSize >= size, "size can't be bigger than the current size!");

		_currentSize -= size;

		if(_currentSize == 0)
		{
			// If this gets rid of every single allocation, we can just short circuit the whole thing
			Reset();
			return;
		}

		do {

			// put_back() doesn't say how much/little can be put back,
			// so in the worst case we need to walk a little bit through the allocation stack

			Allocation &alloc = _allocationStack[_activeSlice];
			size_t slice = std::min(alloc.offset, size);

			alloc.offset -= slice;
			size -= slice;

			if(alloc.offset == 0 && size > 0)
				_activeSlice --;

		} while(size > 0);
	}
	void BumpAllocator::Reset()
	{
		_currentSize = 0;
		_activeSlice = 0;

		// If we have more than 1 allocation on the stack, find the largest allocation and save that
		// Then discard all the other ones
		if(!_allocationStack.empty())
		{
			auto iterator = std::max_element(_allocationStack.begin(), _allocationStack.end());
			Allocation alloc(*iterator);

			_allocationStack.erase(iterator);

			for(auto &alloc : _allocationStack)
				delete[] alloc.memory;

			_allocationStack.clear();
			_allocationStack.push_back(alloc);
		}


		auto &alloc = _allocationStack.front();

		// If the allocation already fits our needs, just reset its offset and carry on
		if(alloc.size >= _maxSize)
		{
			alloc.offset = 0;
			return;
		}


		delete[] alloc.memory;

		alloc.size = _maxSize;
		alloc.memory = new uint8[alloc.size];
	}

	void BumpAllocator::AddAllocation(size_t size)
	{
		size = AlignUp(std::min(size, kBumpAllocatorMultiplier), kBumpAllocatorMultiplier);

		uint8 *buffer = new uint8[size];
		Allocation alloc = { buffer, size, 0 };

		_allocationStack.push_back(alloc);
	}
}
