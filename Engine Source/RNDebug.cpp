//
//  RNDebug.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNDebug.h"

namespace RN
{
	TimeProfiler::TimeProfiler()
	{
		_lastHit = std::chrono::high_resolution_clock::now();
		_accumulated = std::chrono::nanoseconds(0);
		_totalHits = 0;
	}
	
	TimeProfiler::~TimeProfiler()
	{
	}
	
	void TimeProfiler::DumpStatistic()
	{
#ifndef NDEBUG
		{
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(_accumulated).count();
			auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(_accumulated).count();
			
			milliseconds -= (seconds * 1000);
			
			printf("%i total hits, %i:%03i total time\n", (int)_totalHits, (int)seconds, (int)milliseconds);
		}
		
		for(machine_uint i=0; i<_milestones.Count(); i++)
		{
			Milestone& milestone = _milestones[(int)i];
			
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(milestone.accumulated).count();
			auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(milestone.accumulated).count();
			milliseconds -= (seconds * 1000);
			
			auto fseconds = std::chrono::duration_cast<std::chrono::seconds>(milestone.firstHit).count();
			auto fmilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(milestone.firstHit).count();
			fmilliseconds -= (fseconds * 1000);
			
			printf("Milestone \"%s\": %i hit(s) %i:%03i, first hit at %i:%03i in\n", milestone.name.c_str(), milestone.hits, (int)seconds, (int)milliseconds, (int)fseconds, (int)fmilliseconds);
		}
#endif
	}
	
	void TimeProfiler::FinishedMilestone(const std::string& name)
	{
#ifndef NDEBUG
		std::chrono::time_point<std::chrono::high_resolution_clock> hit = std::chrono::high_resolution_clock::now();
		std::chrono::nanoseconds duration = std::chrono::duration_cast<std::chrono::nanoseconds>(hit - _lastHit);
		
		_accumulated += duration;
		
		for(machine_uint i=0; i<_milestones.Count(); i++)
		{
			Milestone& milestone = _milestones[(int)i];
			if(milestone.name.compare(name) == 0)
			{
				milestone.accumulated += duration;
				break;
			}
		}
		
		_lastHit = std::chrono::high_resolution_clock::now();
#endif
	}
	
	void TimeProfiler::HitMilestone(const std::string& name, bool mergeSet)
	{
#ifndef NDEBUG
		std::chrono::time_point<std::chrono::high_resolution_clock> hit = std::chrono::high_resolution_clock::now();
		std::chrono::nanoseconds duration = std::chrono::duration_cast<std::chrono::nanoseconds>(hit - _lastHit);
		
		_totalHits ++;
		_accumulated += duration;
		
		if(mergeSet)
		{
			for(machine_uint i=0; i<_milestones.Count(); i++)
			{
				Milestone& milestone = _milestones[(int)i];
				if(milestone.name.compare(name) == 0)
				{
					milestone.accumulated += duration;
					milestone.hits ++;
					
					_lastHit = std::chrono::high_resolution_clock::now();
					return;
				}
			}
		}
		
		Milestone milestone;
		milestone.name = name;
		milestone.hits = 1;
		milestone.accumulated = duration;
		milestone.firstHit = _accumulated;
		
		_milestones.AddObject(milestone);
		_lastHit = std::chrono::high_resolution_clock::now();
#endif
	}
}
