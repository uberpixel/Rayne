//
//  RNWorkQueue.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WORKQUEUE_H__
#define __RAYNE_WORKQUEUE_H__

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"

namespace RN
{
	class WorkQueue : public Object
	{
	public:
		RN_OPTIONS(Flags, uint32,
				   Concurrent = (1 << 0),
				   Overcommit = (1 << 1));

		WorkQueue(Flags flags);

		void Suspend();
		void Resume();

	private:
		Flags _flags;

		size_t _width;
		size_t _open;
	};
}

#endif /* __RAYNE_WORKQUEUE_H__ */
