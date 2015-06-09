//
//  RNKernel.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNKernel.h"
#include "RNBaseInternal.h"
#include "../System/RNScreen.h"

namespace RN
{
	static Kernel *__sharedInstance = nullptr;

	Kernel::Kernel(Application *application) :
		_exit(false),
		_application(application)
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

		Screen::InitializeScreens();

		_fileManager = new FileManager();
		_firstFrame = true;
		_frames = 0;

		_delta = 0;
		_time = 0;

		SetMaxFPS(60);

		_application->WillFinishLaunching(this);
	}
	void Kernel::FinishBootstrap()
	{
		_application->DidFinishLaunching(this);
	}

	void Kernel::TearDown()
	{
		_application->WillExit();

		Screen::TeardownScreens();
		WorkQueue::TearDownQueues();

		delete _fileManager;
		delete this;

		__sharedInstance = nullptr;
	}

	void Kernel::SetMaxFPS(uint32 maxFPS)
	{
		_maxFPS = maxFPS;
		_minDelta = 1.0 / maxFPS;
	}

	void Kernel::HandleObserver(RunLoopObserver *observer, RunLoopObserver::Activity activity)
	{
		if(RN_EXPECT_FALSE(_exit))
		{
			_runLoop->Stop();
			return;
		}

		Clock::time_point now = Clock::now();

		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastFrame).count();
		_delta = milliseconds / 1000.0;


		if(RN_EXPECT_FALSE(_firstFrame))
		{
			FinishBootstrap();
			_delta = 0.0;

			_firstFrame = false;
		}

		_time += _delta;

		_application->WillStep(_delta);

		// Perform work submitted to the main queue
		{
			volatile bool finishWork;
			_mainQueue->Perform([&]{ finishWork = true; });

			do {
				finishWork = false;
				_mainQueue->PerformWork();
			} while(!finishWork);
		}

		// System event handling
#if RN_PLATFORM_MAC_OS
		@autoreleasepool {

			NSDate *date = [NSDate date];
			NSEvent *event;

			while((event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:date inMode:NSDefaultRunLoopMode dequeue:YES]))
			{
				[NSApp sendEvent:event];
				[NSApp updateWindows];
			}
		}
#endif

		_application->DidStep(_delta);
		_lastFrame = now;

		// FPS cap
		if(_maxFPS > 0)
		{
			now = Clock::now();

			milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastFrame).count();
			double delta = milliseconds / 1000.0;

			if(_minDelta > delta)
			{
				uint32 sleepTime = static_cast<uint32>((_minDelta - delta) * 1000000);
				if(sleepTime > 1000)
					std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
			}
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
