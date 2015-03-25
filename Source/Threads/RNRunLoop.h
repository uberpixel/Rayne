//
//  RNRunLoop.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RUNLOOP_H__
#define __RAYNE_RUNLOOP_H__

#include "../Base/RNBase.h"
#include "../Base/RNFunction.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNSet.h"

namespace RN
{
	class RunLoop;

	class RunLoopObserver : public Object
	{
	public:
		RN_OPTIONS(Activity, uint32,
				   Entry = (1 << 0),
				   Sources = (1 << 2),
				   Finalize = (1 << 3),
				   BeforeWaiting = (1 << 4),
				   AfterWaiting = (1 << 5),
				   Exit = (1 << 6));

		RNAPI RunLoopObserver(Activity activites, bool repeats, std::function<void (RunLoopObserver *, Activity)> &&callback);

		RNAPI void Invalidate();

		Activity GetActivities() const { return _activites; }
		const std::function<void (RunLoopObserver *, Activity)> &GetCallback() const { return _callback; }
		bool IsRepeating() const { return _repeats; }
		bool IsValid() const { return _valid; }

	private:
		Activity _activites;
		std::function<void (RunLoopObserver *, Activity)> _callback;
		bool _repeats;
		bool _valid;

		RNDeclareMeta(RunLoopObserver)
	};

	class RunLoopSource : public Object
	{
	public:
		friend class RunLoop;

		RNAPI void Invalidate();
		RNAPI void Signal();

		bool IsValid() const { return _valid; }
		bool IsSignaled() const { return _signaled; }

	protected:
		RNAPI RunLoopSource();

		RNAPI virtual void WasAdded();
		RNAPI virtual void WasRemoved();

		RNAPI virtual void Perform() = 0;

	private:
		void __AddToRunLoop(RunLoop *runLoop);
		void __RemoveFromRunLoop();

		std::atomic<bool> _valid;
		std::atomic<bool> _signaled;

		std::mutex _lock;
		RunLoop *_runLoop;

		RNDeclareMeta(RunLoopSource)
	};


	class RunLoop
	{
	public:
		using Clock = std::chrono::steady_clock;
		using Duration = std::chrono::microseconds;

		RNAPI RunLoop();
		RNAPI ~RunLoop();

		RNAPI static RunLoop *GetMainRunLoop();
		RNAPI static RunLoop *GetCurrentRunLoop();

		RNAPI void AddObserver(RunLoopObserver *observer);
		RNAPI void RemoveObserver(RunLoopObserver *observer);

		RNAPI void AddSource(RunLoopSource *source);
		RNAPI void RemoveSource(RunLoopSource *source);

		RNAPI void Run();
		RNAPI bool RunUntil(const Clock::time_point &timeout);
		RNAPI void Stop();
		RNAPI void WakeUp();

	private:
		bool Step(const Clock::time_point &now);

		void DoObservers(RunLoopObserver::Activity activity);
		bool DoSources();

		void CollectObservers();
		void CollectSources();

		Clock::time_point GetNextWakeUp();

		Array *_observers;
		Set *_sources;

		std::atomic<bool> _needsWakeup;
		std::atomic<bool> _stopped;
		std::atomic<bool> _waiting;

		std::mutex _lock;
		std::condition_variable _signal;
	};
}

#endif /* __RAYNE_RUNLOOP_H__ */
