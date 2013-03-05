//
//  RNDebug.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_DEBUG_H__
#define __RAYNE_DEBUG_H__

#include "RNBase.h"
#include "RNArray.h"

namespace RN
{
	class TimeProfiler
	{
	public:
		TimeProfiler();
		~TimeProfiler();
		
		void DumpStatistic();
		void HitMilestone(const std::string& name, bool mergeSet=true);
		void FinishedMilestone(const std::string& name);
		
	private:
		struct Milestone
		{
			std::string name;
			std::chrono::nanoseconds accumulated;
			std::chrono::nanoseconds firstHit;
			uint32 hits;
		};
		
		Array<Milestone> _milestones;
		std::chrono::time_point<std::chrono::high_resolution_clock> _lastHit;
		std::chrono::nanoseconds _accumulated;
		machine_uint _totalHits;
	};
	
	class Timer
	{
	public:
		Timer();
		~Timer();
		
		float Time();
		
	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> _time;
	};
}

#endif /* __RAYNE_DEBUG_H__ */
