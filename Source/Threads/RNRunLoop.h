//
//  RNRunLoop.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RUNLOOP_H__
#define __RAYNE_RUNLOOP_H__

#include <chrono>
#include "../Base/RNBase.h"

namespace RN
{
	class RunLoop
	{
	public:
		using Clock = std::chrono::steady_clock;
		using Duration = std::chrono::microseconds;

		void Run();
		bool RunUntil(const Clock::time_point &timeout);

	private:
		bool Step(const Clock::time_point &now);

		Duration GetNextWakeUp();


	};
}

#endif /* __RAYNE_RUNLOOP_H__ */
