//
//  RNTimer.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_TIMER_H__
#define __RAYNE_TIMER_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNFunction.h"

namespace RN
{
	class Timer : public Object
	{
	public:
		RNAPI Timer(std::chrono::milliseconds duration, Function &&function, bool repeats = false);
		RNAPI Timer(std::chrono::steady_clock::time_point date, Function &&function, bool repeats = false);
		
		RNAPI static Timer *ScheduledTimerWithDuration(std::chrono::milliseconds duration, Function &&function, bool repeats);
		RNAPI static Timer *ScheduledTimerWithFireDate(std::chrono::steady_clock::time_point date, Function &&function, bool repeats);
		
		RNAPI void Fire();
		RNAPI void Invalidate();
		
		RNAPI std::chrono::steady_clock::time_point GetFireDate() const { return _date; }
		RNAPI std::chrono::milliseconds GetDuration() const { return _duration; }
		RNAPI bool IsValid() const { return _valid; }
		
	private:
		Function _callback;
		
		std::chrono::steady_clock::time_point _date;
		std::chrono::milliseconds _duration;
		
		bool _valid;
		bool _repeats;
		
		RNDeclareMeta(Timer)
	};
	
	RNObjectClass(Timer)
}

#endif /* __RAYNE_TIMER_H__ */
