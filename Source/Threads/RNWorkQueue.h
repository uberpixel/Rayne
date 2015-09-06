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
#include "../Data/RNRingbuffer.h"
#include "RNWorkSource.h"
#include "RNThread.h"

namespace RN
{
	class Kernel;

	class WorkQueue : public Object
	{
	public:
		friend class Kernel;

		RN_OPTIONS(Flags, uint32,
				   Concurrent = (1 << 0));

		enum class Priority : uint32
		{
			High = 0,
			Default = 1,
			Background = 2
		};

		RNAPI WorkQueue(Priority priority, Flags flags);
		RNAPI ~WorkQueue();

		RNAPI static WorkQueue *GetMainQueue();
		RNAPI static WorkQueue *GetGlobalQueue(Priority priority);

		RNAPI void Perform(Function &&function);
		RNAPI void PerformBarrier(Function &&function);
		RNAPI void PerformSynchronous(Function &&function);
		RNAPI void PerformSynchronousBarrier(Function &&function);

		RNAPI void Suspend();
		RNAPI void Resume();

	private:
		struct WorkThread
		{
			WorkThread() :
				sleeping(false),
				thread(nullptr),
				localOpen(0)
			{}

			std::atomic<bool> sleeping;
			std::atomic<Thread *> thread;
			AtomicRingBuffer<WorkSource *, 512> localBuffer;
			std::atomic<size_t> localOpen;
			std::condition_variable workSignal;
			std::mutex workLock;
		};

		static void InitializeQueues();
		static void TearDownQueues();

		WorkSource *PerformWithFlags(Function &&function, WorkSource::Flags flags);

		void ThreadEntry(WorkThread *thread);
		bool PerformWork(WorkThread *thread);
		bool PerformWorkMainThread();

		void ReCalculateWidth();

		Flags _flags;

		size_t _concurrency;
		size_t _threshold;

		size_t _width;
		size_t _realWidth;

		std::atomic<size_t> _open;
		std::atomic<size_t> _running;
		//std::atomic<size_t> _sleeping;
		std::atomic<size_t> _suspended;
		std::atomic<bool> _barrier;

		std::condition_variable _barrierSignal;
		std::mutex _barrierLock;

		//std::condition_variable _workSignal;
		//std::mutex _workLock;

		std::condition_variable _syncSignal;
		std::mutex _syncLock;

		/*SpinLock _lock;
		WorkSource *_workHead;
		WorkSource *_workTail;

		AtomicRingBuffer<WorkSource *, 512> _buffer;
		std::vector<WorkSource *> _overcommit;
		std::atomic<bool> _isOverCommitted;*/

		SpinLock _threadLock;
		std::vector<WorkThread *> _threads;

		RNDeclareMeta(WorkQueue)
	};

	RNObjectClass(WorkQueue)
}

#endif /* __RAYNE_WORKQUEUE_H__ */
