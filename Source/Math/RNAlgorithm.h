//
//  RNAlgorithm.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ALGORITHM_H__
#define __RAYNE_ALGORITHM_H__

#include "../Base/RNBase.h"

#define kRNParallelSortMaxBatchSize  10
#define kRNParallelSortMinBatchCount 4

namespace RN
{
	template<class T>
	void HashCombine(size_t &seed, const T &value)
	{		
		// This function is equivalent to boost::hash_combine()
		std::hash<T> hasher;
		seed ^= static_cast<size_t>(hasher(value)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
	
	template<class Iterator>
	size_t HashRange(Iterator first, Iterator last)
	{
		size_t hash = 0;
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
