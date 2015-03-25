//
//  RNWorkQueue.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorkQueue.h"

namespace RN
{
	WorkQueue::WorkQueue(Flags flags) :
		_flags(flags),
		_width(1),
		_open(0)
	{}

	void WorkQueue::Suspend()
	{}
	void WorkQueue::Resume()
	{

	}
}
