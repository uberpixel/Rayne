//
//  RNWorkSource.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RANYE_WORKSOURCE_H__
#define __RANYE_WORKSOURCE_H__

#include "../Base/RNBase.h"

namespace RN
{
	struct WorkSourcePool;
	struct WorkSource
	{
	public:
		RN_OPTIONS(Flags, uint32,
				   Barrier = (1 << 0),
				   Synchronous = (1 << 1));

		static WorkSource *DequeueWorkSource(Function &&function, Flags flags);

		bool TestFlag(Flags flags) const { return (_flags & flags); }
		bool IsComplete() const { return _completed; }

		void Callout() { _function(); }
		void Complete() { _completed = true; }
		void Relinquish();

	private:
		WorkSource(Function &&function, Flags flags, WorkSourcePool *pool);
		void Refurbish(Function &&function, Flags flags);

		Function _function;
		Flags _flags;
		WorkSourcePool *_pool;
		std::atomic<bool> _completed;
	};
}

#endif /* __RANYE_WORKSOURCE_H__ */
