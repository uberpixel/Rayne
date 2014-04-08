//
//  RNTimer.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNTimer.h"
#include "RNKernel.h"

namespace RN
{
	RNDefineMeta(Timer, Object)
	
	Timer::Timer(std::chrono::milliseconds duration, Function &&function, bool repeats) :
		_callback(std::move(function)),
		_duration(duration),
		_repeats(repeats),
		_valid(true)
	{
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		_date = now + _duration;
	}
	
	Timer::Timer(std::chrono::steady_clock::time_point date, Function &&function, bool repeats) :
		_callback(std::move(function)),
		_date(date),
		_repeats(repeats),
		_valid(true)
	{
		_duration = std::chrono::duration_cast<std::chrono::milliseconds>(_date - date);
		
		if(_duration < std::chrono::milliseconds(0))
			_duration = -_duration;
	}
	
	Timer *Timer::ScheduledTimerWithDuration(std::chrono::milliseconds duration, Function &&function, bool repeats)
	{
		Timer *timer = new Timer(duration, std::move(function), repeats);
		Kernel::GetSharedInstance()->ScheduleTimer(timer);
		return timer->Autorelease();
	}
	
	Timer *Timer::ScheduledTimerWithFireDate(std::chrono::steady_clock::time_point date, Function &&function, bool repeats)
	{
		Timer *timer = new Timer(date, std::move(function), repeats);
		Kernel::GetSharedInstance()->ScheduleTimer(timer);
		return timer->Autorelease();
	}
	
	
	void Timer::Fire()
	{
		RN_ASSERT(_valid, "Timer needs to be valid to be fired!");
		
		_callback();
		
		if(_repeats)
		{
			std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
			
			_date = _date + _duration;
			
			if(_date <= now)
				_date = now + _duration;
		}
		else
		{
			Invalidate();
		}
	}
	
	void Timer::Invalidate()
	{
		_valid = false;
		Kernel::GetSharedInstance()->RemoveTimer(this);
	}
}
