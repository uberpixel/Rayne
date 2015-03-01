//
//  RNRunLoop.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <thread>
#include <algorithm>

#include "RNRunLoop.h"

namespace RN
{
	void RunLoop::Run()
	{
		while(RunUntil(Clock::time_point::max()))
		{}
	}
	bool RunLoop::RunUntil(const Clock::time_point &timeout)
	{
		bool result;

		do {
			auto now = Clock::now();

			result = Step(now);
			if(result)
			{
				auto wakeup = std::min(GetNextWakeUp(), std::chrono::duration_cast<Duration>(timeout - now));
				std::this_thread::sleep_for(wakeup);
			}
		} while(result && Clock::now() > timeout);

		return result;
	}

	bool RunLoop::Step(const Clock::time_point &now)
	{
		return false;
	}

	std::chrono::microseconds RunLoop::GetNextWakeUp()
	{
		return std::chrono::microseconds(0);
	}
}
