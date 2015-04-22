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
	class WorkSource;
	class WorkQueue : public Object
	{
	public:
		RN_OPTIONS(Flags, uint32,
				   Concurrent = (1 << 0));

		enum class Priority : uint32
		{
			High = 0,
			Default = 1,
			Background = 2
		};

		WorkQueue(Priority priority, Flags flags);

		static WorkQueue *GetQueueWithPriority(Priority priority);

		void AddWork(Function &&function);
		void AddWorkBarrier(Function &&function);
		void AddWorkSynchronous(Function &&function);
		void AddWorkSynchronousBarrier(Function &&function);

		void Suspend();
		void Resume();

	private:
		WorkSource *AddWorkWithFlags(Function &&function, WorkSource::Flags flags);

		void ThreadEntry();
		void ReCalculateWidth();

		Flags _flags;

		size_t _concurrency;
		size_t _threshold;

		size_t _width;
		size_t _realWidth;

		std::atomic<size_t> _open;
		std::atomic<size_t> _running;
		std::atomic<size_t> _sleeping;
		std::atomic<size_t> _suspended;
		std::atomic<bool> _barrier;

		std::condition_variable _barrierSignal;
		std::mutex _barrierLock;

		std::condition_variable _workSignal;
		std::mutex _workLock;

		std::condition_variable _syncSignal;
		std::mutex _syncLock;

		SpinLock _readLock;
		SpinLock _writeLock;

		AtomicRingBuffer<WorkSource *, 512> _buffer;
		std::vector<WorkSource *> _overcommit;
		std::atomic<bool> _isOverCommitted;

		SpinLock _threadLock;
		std::vector<Thread *> _threads;
	};
}

#endif /* __RAYNE_WORKQUEUE_H__ */
