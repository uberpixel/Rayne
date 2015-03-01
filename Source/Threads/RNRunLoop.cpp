//
//  RNRunLoop.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRunLoop.h"
#include "RNThread.h"

namespace RN
{
	RNDefineMeta(RunLoopObserver, Object)
	RNDefineMeta(RunLoopSource, Object)

	// ---------------------------
	// RunLoopObserver
	// ---------------------------

	RunLoopObserver::RunLoopObserver(Activity activites, bool repeats, std::function<void (RunLoopObserver *, Activity)> &&callback) :
		_activites(activites),
		_repeats(repeats),
		_callback(std::move(callback)),
		_valid(true)
	{}

	void RunLoopObserver::Invalidate()
	{
		_valid = false;
	}

	// ---------------------------
	// RunLoopSource
	// ---------------------------

	RunLoopSource::RunLoopSource() :
		_signaled(false),
		_valid(true),
		_runLoop(nullptr)
	{}

	void RunLoopSource::Invalidate()
	{
		std::unique_lock<std::mutex> lock(_lock);
		_valid = false;
	}

	void RunLoopSource::Signal()
	{
		std::unique_lock<std::mutex> lock(_lock);
		_signaled = true;

		if(_runLoop)
			_runLoop->WakeUp();
	}

	void RunLoopSource::__AddToRunLoop(RunLoop *runLoop)
	{
		std::unique_lock<std::mutex> lock(_lock);

		_runLoop = runLoop;
		WasAdded();
	}
	void RunLoopSource::__RemoveFromRunLoop()
	{
		std::unique_lock<std::mutex> lock(_lock);

		_runLoop = nullptr;
		WasRemoved();
	}

	void RunLoopSource::WasAdded()
	{}
	void RunLoopSource::WasRemoved()
	{}

	// ---------------------------
	// RunLoop
	// ---------------------------

	RunLoop::RunLoop() :
		_observers(new Array()),
		_sources(new Set()),
		_needsWakeup(false),
		_stopped(false)
	{}

	RunLoop::~RunLoop()
	{
		_observers->Release();
		_sources->Release();
	}

	RunLoop *RunLoop::GetMainRunLoop()
	{
		return Thread::GetMainThread()->GetRunLoop();
	}

	RunLoop *RunLoop::GetCurrentRunLoop()
	{
		return Thread::GetCurrentThread()->GetRunLoop();
	}

	void RunLoop::AddObserver(RunLoopObserver *observer)
	{
		_observers->AddObject(observer);
	}
	void RunLoop::RemoveObserver(RunLoopObserver *observer)
	{
		_observers->RemoveObject(observer);
	}

	void RunLoop::AddSource(RunLoopSource *source)
	{
		RN_ASSERT(source->_runLoop == nullptr, "Source mustn't have been added to a run loop");

		_sources->AddObject(source);
		source->__AddToRunLoop(this);
	}

	void RunLoop::RemoveSource(RunLoopSource *source)
	{
		if(source->_runLoop == this)
		{
			source->__RemoveFromRunLoop();
			_sources->RemoveObject(source);
		}
	}

	void RunLoop::Run()
	{
		while(RunUntil(Clock::time_point::max()))
		{}
	}
	bool RunLoop::RunUntil(const Clock::time_point &timeout)
	{
		bool result;

		_stopped = false;

		DoObservers(RunLoopObserver::Activity::Entry);

		do {
			auto now = Clock::now();

			_needsWakeup = false;

			result = Step(now);
			CollectObservers();
			CollectSources();

			if(result)
			{
				if(_needsWakeup)
					continue;

				DoObservers(RunLoopObserver::Activity::BeforeWaiting);

				auto wakeup = std::min(GetNextWakeUp(), timeout);

				std::unique_lock<std::mutex> lock(_lock);
				_waiting = true;
				_signal.wait_until(lock, wakeup, [&]() -> bool { return _needsWakeup; });
				_waiting = false;

				DoObservers(RunLoopObserver::Activity::AfterWaiting);
			}
		} while(result && !_stopped && Clock::now() > timeout);

		DoObservers(RunLoopObserver::Activity::Exit);

		return result;
	}

	void RunLoop::Stop()
	{
		std::lock_guard<std::mutex> lock(_lock);
		_stopped = true;

		if(_waiting)
			WakeUp();
	}

	void RunLoop::WakeUp()
	{
		_needsWakeup = true;
		_signal.notify_one();
	}

	bool RunLoop::Step(const Clock::time_point &now)
	{
		DoObservers(RunLoopObserver::Activity::Sources);
		DoSources();

		DoObservers(RunLoopObserver::Activity::Finalize);

		return (_sources->GetCount() > 0);
	}

	void RunLoop::DoObservers(RunLoopObserver::Activity activity)
	{
		size_t count = _observers->GetCount();
		if(count == 0)
			return;

		RunLoopObserver **collected = static_cast<RunLoopObserver **>(alloca(count * sizeof(RunLoopObserver *)));
		size_t collectedCount = 0;

		_observers->Enumerate<RunLoopObserver>([&](RunLoopObserver *observer, size_t index, bool &stop) {

			if(observer->IsValid() && observer->GetActivities() & activity)
				collected[collectedCount ++] = observer->Retain();

		});

		for(size_t i = 0; i < collectedCount; i ++)
		{
			RunLoopObserver *observer = collected[i];

			observer->GetCallback()(observer, activity);
			if(!observer->IsRepeating())
				observer->Invalidate();

			observer->Release();
		}
	}

	bool RunLoop::DoSources()
	{
		if(_sources->GetCount() == 0)
			return false;

		Array *pending = new Array();

		_sources->Enumerate<RunLoopSource>([&](RunLoopSource *source, bool &stop) {

			if(source->IsValid() && source->IsSignaled())
				pending->AddObject(source);

		});

		if(pending->GetCount() > 0)
		{
			pending->Enumerate<RunLoopSource>([&](RunLoopSource *source, size_t index, bool &stop) {

				source->_signaled = false;
				source->Perform();

			});

			pending->Release();
			return true;
		}

		pending->Release();
		return false;
	}

	void RunLoop::CollectObservers()
	{
		size_t count = _observers->GetCount();
		if(count == 0)
			return;

		RunLoopObserver **collected = static_cast<RunLoopObserver **>(alloca(count * sizeof(RunLoopObserver *)));
		size_t collectedCount = 0;

		_observers->Enumerate<RunLoopObserver>([&](RunLoopObserver *observer, size_t index, bool &stop) {

			if(!observer->IsValid())
				collected[collectedCount ++] = observer;

		});

		for(size_t i = 0; i < collectedCount; i ++)
		{
			RunLoopObserver *observer = collected[i];
			_observers->RemoveObject(observer);
		}
	}

	void RunLoop::CollectSources()
	{
		if(_sources->GetCount() == 0)
			return;

		Array *dead = new Array();

		_sources->Enumerate<RunLoopSource>([&](RunLoopSource *source, bool &stop) {

			if(!source->IsValid())
				dead->AddObject(source);

		});

		dead->Enumerate<RunLoopSource>([&](RunLoopSource *source, size_t index, bool &stop) {
			_sources->RemoveObject(source);
		});

		dead->Release();
	}

	RunLoop::Clock::time_point RunLoop::GetNextWakeUp()
	{
		return Clock::time_point::max();
	}
}
