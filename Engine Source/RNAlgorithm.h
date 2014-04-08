//
//  RNAlgorithm.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ALGORITHM_H__
#define __RAYNE_ALGORITHM_H__

#include "RNBase.h"
#include "RNThreadPool.h"

#define kRNParallelSortMaxBatchSize  10
#define kRNParallelSortMinBatchCount 4

namespace RN
{
	template<class Iterator, class Compare>
	void ParallelSort(Iterator first, Iterator last, Compare comp)
	{
		size_t count   = std::distance(first, last);
		size_t threads = ThreadCoordinator::GetSharedInstance()->GetBaseConcurrency();
		
		if((count / threads) > kRNParallelSortMinBatchCount)
		{
			size_t batches = count / kRNParallelSortMaxBatchSize;
			ThreadPool::Batch *batch = ThreadPool::GetSharedInstance()->CreateBatch();
			
			auto begin = first;
			
			for(size_t i=0; i<batches; i++)
			{
				auto end = begin;
				std::advance(end, kRNParallelSortMaxBatchSize);
				
				batch->AddTask([&, begin, end] { std::sort(begin, end, comp); });
				
				begin = end;
			}
			
			if(std::distance(begin, last) > 0)
			{
				batch->AddTask([&, begin, last] { std::sort(begin, last, comp); });
			}
			
			batch->Commit();
			batch->Wait();
			batch->Release();
		}
		
		std::sort(first, last, comp);
	}
	
	template<class T>
	void HashCombine(machine_hash& seed, const T& value)
	{		
		// This function is equivalent to boost::hash_combine()
		std::hash<T> hasher;
		seed ^= static_cast<machine_hash>(hasher(value)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
	
	template<class Iterator>
	machine_hash HashRange(Iterator first, Iterator last)
	{
		machine_hash hash = 0;
		for(; first != last; first ++)
		{
			HashCombine(hash, *first);
		}
		
		return hash;
	}
	
	static inline uint32 NextPowerOfTwo(uint32 value)
	{
		value--;
		
		value |= value >> 1;
		value |= value >> 2;
		value |= value >> 4;
		value |= value >> 8;
		value |= value >> 16;
		
		return (value + 1);
	}
	
	static inline uint64 NextPowerOfTwo(uint64 value)
	{
		value--;
		
		value |= value >> 1;
		value |= value >> 2;
		value |= value >> 4;
		value |= value >> 8;
		value |= value >> 16;
		value |= value >> 32;
		
		return (value + 1);
	}
	
	template<class T>
	static inline bool IsPowerOfTwo(T value)
	{
		static_assert(std::is_integral<T>::value, "T must be integral");
		return ((value & (value - 1)) == 0);
	}
}

#endif /* __RAYNE_ALGORITHM_H__ */
