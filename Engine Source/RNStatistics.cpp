//
//  RNStatistics.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNStatistics.h"

namespace RN
{
	Statistics::Statistics()
	{}
	
	Statistics::~Statistics()
	{
		for(DataPoint *point : _totalPoints)
		{
			delete point;
		}
	}
	
	
	
	Statistics::DataPoint *Statistics::GetPointForKey(const std::string& key)
	{
		auto iterator = _dataPoints.find(key);
		if(iterator == _dataPoints.end())
		{
			DataPoint *point = new DataPoint();
			point->name     = key;
			point->duration = std::chrono::microseconds::zero();
			point->open     = 0;
			
			_dataPoints.insert(decltype(_dataPoints)::value_type(key, point));
			_totalPoints.push_back(point);
			
			std::sort(_totalPoints.begin(), _totalPoints.end(), [](const DataPoint *point1, const DataPoint *point2) {
				return (point1->name < point2->name);
			});
			
			return point;
		}
		
		return iterator->second;
	}
	
	void Statistics::Push(const std::string& name)
	{
		DataPoint *point = GetPointForKey(name);
		
		if((point->open ++) != 0)
			return;
		
		point->last  = std::chrono::high_resolution_clock::now();
		point->open  = true;
		point->decay = 30;
		
		_openPoints.push_back(point);
	}
	
	void Statistics::Pop()
	{
		DataPoint *point = _openPoints.back();
		
		if((-- point->open) > 0)
			return;
		
		point->duration += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - point->last);
		point->open = false;
		
		_openPoints.pop_back();
	}
	
	void Statistics::Clear()
	{
		RN_ASSERT(_openPoints.empty(), "All pushed points must be balanced by pops!");
		
		std::vector<DataPoint *> decayed;
		
		for(DataPoint *point : _totalPoints)
		{
			point->duration = std::chrono::microseconds::zero();
			
			if((point->decay --) == 0)
				decayed.push_back(point);
		}
		
		if(!decayed.empty())
		{
			for(DataPoint *point : decayed)
			{
				_totalPoints.erase(std::find(_totalPoints.begin(), _totalPoints.end(), point));
				_dataPoints.erase(point->name);
				
				delete point;
			}
		}
	}
}
