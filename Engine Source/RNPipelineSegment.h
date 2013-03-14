//
//  RNPipelineSegment.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PIPELINESEGMENT_H__
#define __RAYNE_PIPELINESEGMENT_H__

#include "RNBase.h"
#include "RNThread.h"
#include "RNMutex.h"
#include "RNDebug.h"

#define kPipelineSegmentNullTask ((uint32)-1)

namespace RN
{	
	class PipelineSegment
	{
	public:
		typedef uint32 TaskID;
		
		PipelineSegment(bool spinThread);
		virtual ~PipelineSegment();
		
		TaskID BeginTask(float delta);
		void WaitForTaskCompletion(TaskID task);
		Thread *WorkerThread() const { return _thread; }
		
		float UsedTime() const { return _usedTime; }
		
	protected:
		bool WaitForWork();
		void SetThread(Thread *thread);
		void Exit();
		TaskID CurrentTask() const { return _task; }
		
		virtual void WorkOnTask(TaskID task, float delta) = 0;
		virtual void WorkLoop();
		
	private:
		void WorkerLoop();
		
		Thread *_thread;
		float _delta;
		
		TaskID _task;
		TaskID _lastTask;
		
		Timer _timer;
		float _usedTime;
		
		std::condition_variable _waitCondition;
		std::condition_variable _workCondition;
		
		std::mutex _waitMutex;
		std::mutex _workMutex;
	};
}

#endif /* __RAYNE_PIPELINESEGMENT_H__ */
