//
//  RNAlgorithm.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		size_t threads = ThreadCoordinator::SharedInstance()->BaseConcurrency();
		
		if((count / threads) > kRNParallelSortMinBatchCount)
		{
			size_t batches = count / kRNParallelSortMaxBatchSize;
			ThreadPool::Batch batch = ThreadPool::SharedInstance()->OpenBatch();
			
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
		}
		
		std::sort(first, last, comp);
	}
}

#endif /* __RAYNE_ALGORITHM_H__ */
