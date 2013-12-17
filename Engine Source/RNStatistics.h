//
//  RNStatistics.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STATISTICS_H__
#define __RAYNE_STATISTICS_H__

#include "RNBase.h"

namespace RN
{
	class Statistics
	{
	public:
		struct DataPoint
		{
			std::string name;
			std::chrono::microseconds duration;
			
			uint32 open;
			uint32 decay;
			std::chrono::high_resolution_clock::time_point last;
		};
		
		Statistics();
		~Statistics();
		
		void Push(const std::string& name);
		void Pop();
		
		void Clear();
		
		const std::vector<DataPoint *>& GetDataPoints() const { return _totalPoints; }
		
	private:
		DataPoint *GetPointForKey(const std::string& key);
		
		std::unordered_map<std::string, DataPoint *> _dataPoints;
		std::vector<DataPoint *> _totalPoints;
		std::vector<DataPoint *> _openPoints;
	};
}

#endif /* __RAYNE_STATISTICS_H__ */
