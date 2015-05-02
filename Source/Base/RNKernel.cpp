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
	static Kernel *__sharedInstance = nullptr;

	Kernel::Kernel() :
		_exit(false)
	{}

	Kernel::~Kernel()
	{}

	Kernel *Kernel::GetSharedInstance()
	{
		return __sharedInstance;
	}


	void Kernel::Bootstrap()
	{
		__sharedInstance = this;

		WorkQueue::InitializeQueues();

		_observer = new RunLoopObserver(RunLoopObserver::Activity::Finalize, true, std::bind(&Kernel::HandleObserver, this, std::placeholders::_1, std::placeholders::_2));
		_mainThread = new Thread();
		_mainQueue = WorkQueue::GetMainQueue();

		_runLoop = _mainThread->GetRunLoop();
		_runLoop->AddObserver(_observer);

		_fileManager = new FileManager();
	}
	void Kernel::TearDown()
	{
		WorkQueue::TearDownQueues();

		delete _fileManager;
		delete this;

		__sharedInstance = nullptr;
	}


	void Kernel::HandleObserver(RunLoopObserver *observer, RunLoopObserver::Activity activity)
	{
		if(_exit)
		{
			_runLoop->Stop();
			return;
		}

		{
			volatile bool finishWork;
			_mainQueue->Perform([&]{ finishWork = true; });

			do {
				finishWork = false;
				_mainQueue->PerformWork();
			} while(!finishWork);
		}

		// Make sure the run loop wakes up again afterwards
		_runLoop->WakeUp();
	}

	void Kernel::Run()
	{
		_exit = false;

		do {
			_runLoop->Run();
		} while(!_exit);
	}

	void Kernel::Exit()
	{
		_exit = true;
	}
}
