//
//  RNRenderThreadQueue.h
//  Rayne
//
//  Copyright 2026 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERTHREADQUEUE_H_
#define __RAYNE_RENDERTHREADQUEUE_H_

#include "../Base/RNFunction.h"
#include "RNRenderingConfig.h"

namespace RN
{
	template<class T>
	class RenderThreadQueue
	{
	public:
		enum class WorkType
		{
			None,
			FrameSubmission,
			Task
		};

		RenderThreadQueue(bool suspended = false) : _isSuspended(suspended) {}

		// Stop between work items; queued frames and tasks remain owned by the queue.
		// The owner must serialize Suspend and Resume (Shutdown may interrupt either).
		void Suspend(Function &&prepare)
		{
			UniqueLock<Lockable> lock(_lock);
			if(_isShuttingDown || _isSuspended) return;
			_isSuspending = true;
			_workItems.emplace_front(Function([this, prepare = std::move(prepare)]() mutable {
				prepare();
				UniqueLock<Lockable> lock(_lock);
				_isSuspended = true;
				_isSuspending = false;
				_consumedCondition.NotifyAll();
			}));
			_queuedCondition.NotifyOne();
			_consumedCondition.NotifyAll();
			_consumedCondition.Wait(lock, [this]() { return _isShuttingDown || _isSuspended; });
		}

		void Resume()
		{
			UniqueLock<Lockable> lock(_lock);
			_isSuspended = false;
			_queuedCondition.NotifyAll();
			_consumedCondition.NotifyAll();
		}

		bool WaitForSpace()
		{
			RN_PROFILE_SCOPE_N("WaitForRenderFrameQueueSpace");
			RN_PROFILE_ATRACE_SCOPE_N("RN RenderQueue WaitForFrameSlot");
			UniqueLock<Lockable> lock(_lock);
			_consumedCondition.Wait(lock, [this]() {
				return _isShuttingDown || _isSuspending || _isSuspended || _queuedFrameSubmissionCount < RN_RENDERING_FRAME_SUBMISSION_QUEUE_SIZE;
			});

			return !_isShuttingDown && !_isSuspending && !_isSuspended;
		}

		void Push(T &&submission)
		{
			UniqueLock<Lockable> lock(_lock);
			if(_isShuttingDown)
				return;

			RN_ASSERT(_queuedFrameSubmissionCount < RN_RENDERING_FRAME_SUBMISSION_QUEUE_SIZE, "Render thread frame submission queue is full");
			_workItems.emplace_back(std::move(submission));
			_queuedFrameSubmissionCount += 1;
			_queuedCondition.NotifyOne();
		}

		void PushTask(Function &&task)
		{
			UniqueLock<Lockable> lock(_lock);
			if(_isShuttingDown)
				return;

			_workItems.emplace_back(std::move(task));
			_queuedCondition.NotifyOne();
		}

		void Synchronize()
		{
			uint64 synchronizationID;
			{
				UniqueLock<Lockable> lock(_lock);
				if(_isShuttingDown)
					return;

				synchronizationID = ++_nextSynchronizationID;
				_workItems.emplace_back(Function([this, synchronizationID]() {
					UniqueLock<Lockable> lock(_lock);
					if(_completedSynchronizationID < synchronizationID)
						_completedSynchronizationID = synchronizationID;
					_consumedCondition.NotifyAll();
				}));
				_queuedCondition.NotifyOne();
			}

			UniqueLock<Lockable> lock(_lock);
			_consumedCondition.Wait(lock, [this, synchronizationID]() {
				return _isShuttingDown || _completedSynchronizationID >= synchronizationID;
			});
		}

		WorkType Pop(T &submission, Function &task)
		{
			RN_PROFILE_ATRACE_SCOPE_N("RN RenderQueue PopWork");
			UniqueLock<Lockable> lock(_lock);
			_queuedCondition.Wait(lock, [this]() {
				return _isShuttingDown || (!_isSuspended && !_workItems.empty());
			});

			if(_isShuttingDown || _workItems.empty())
				return WorkType::None;

			WorkItem item(std::move(_workItems.front()));
			_workItems.pop_front();

			if(item.type == WorkType::FrameSubmission)
			{
				submission = std::move(item.submission);
				_queuedFrameSubmissionCount -= 1;
				_consumedCondition.NotifyOne();
				return WorkType::FrameSubmission;
			}

			task = std::move(item.task);
			return WorkType::Task;
		}

		void Shutdown()
		{
			UniqueLock<Lockable> lock(_lock);
			_isShuttingDown = true;
			_queuedCondition.NotifyAll();
			_consumedCondition.NotifyAll();
		}

		void Drain()
		{
			UniqueLock<Lockable> lock(_lock);
			_workItems.clear();
			_queuedFrameSubmissionCount = 0;
			_completedSynchronizationID = _nextSynchronizationID;
			_queuedCondition.NotifyAll();
			_consumedCondition.NotifyAll();
		}

	private:
		struct WorkItem
		{
			WorkItem(T &&submission) :
				type(WorkType::FrameSubmission),
				submission(std::move(submission))
			{}

			WorkItem(Function &&task) :
				type(WorkType::Task),
				task(std::move(task))
			{}

			WorkItem(WorkItem &&other) RN_NOEXCEPT = default;
			WorkItem &operator=(WorkItem &&other) RN_NOEXCEPT = default;

			WorkType type;
			T submission;
			Function task;
		};

		std::deque<WorkItem> _workItems;
		Lockable _lock;
		Condition _queuedCondition;
		Condition _consumedCondition;
		size_t _queuedFrameSubmissionCount = 0;
		uint64 _nextSynchronizationID = 0;
		uint64 _completedSynchronizationID = 0;
		bool _isShuttingDown = false;
		bool _isSuspended = false;
		bool _isSuspending = false;
	};
}

#endif /* __RAYNE_RENDERTHREADQUEUE_H_ */
