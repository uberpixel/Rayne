//
//  RNKernel.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNKernel.h"

namespace RN
{
	Kernel::Kernel()
	{
		Bootstrap();
	}

	void Kernel::Bootstrap()
	{
		_observer = new RunLoopObserver(RunLoopObserver::Activity::Finalize, true, std::bind(&Kernel::HandleObserver, this, std::placeholders::_1, std::placeholders::_2));
		_mainThread = new Thread();

		_runLoop = _mainThread->GetRunLoop();
		_runLoop->AddObserver(_observer);

		_exit = false;
	}

	void Kernel::HandleObserver(RunLoopObserver *observer, RunLoopObserver::Activity activity)
	{
		if(_exit)
		{
			_runLoop->Stop();
			return;
		}

		// Make sure the run loop wakes up again afterwards
		_runLoop->WakeUp();
	}

	void Kernel::Run()
	{
		do {
			_runLoop->Run();
		} while(!_exit);
	}
}
