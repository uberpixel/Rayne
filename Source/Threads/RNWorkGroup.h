//
//  RNWorkGroup.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WORKGROUP_H__
#define __RAYNE_WORKGROUP_H__

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "RNWorkQueue.h"

namespace RN
{
	class WorkGroup : public Object
	{
	public:
		RNAPI WorkGroup();
		RNAPI ~WorkGroup();

		RNAPI void Perform(WorkQueue *queue, Function &&function);

		RNAPI void Enter();
		RNAPI void Leave();

		RNAPI void Wait();
		RNAPI bool WaitUntil(const Clock::time_point &timeout);
		RNAPI void Notify(WorkQueue *queue, Function &&function);

	private:
		std::mutex _lock;
		std::condition_variable _signal;
		std::vector<std::pair<WorkQueue *, Function>> _waiters;

		std::atomic<size_t> _open;

		__RNDeclareMetaInternal(WorkGroup)
	};

	RNObjectClass(WorkGroup)
}

#endif /* __RAYNE_WORKGROUP_H__ */
