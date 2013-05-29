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

namespace RN
{
	namespace Algorithm
	{
		template<class Iterator>
		void ParallelSort(Iterator first, Iterator last)
		{
			//size_t count = std::distance(first, last);
			std::sort(first, last);
		}
	}
}

#endif /* __RAYNE_ALGORITHM_H__ */
