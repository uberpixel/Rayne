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

#define kPipelineSegmentNullTask ((uint32)-1)

namespace RN
{	
	class PipelineSegment
	{
	public:
		typedef uint32 TaskID;
		
		PipelineSegment();
		virtual ~PipelineSegment();
		
		TaskID BeginTask(float delta);
		void WaitForTaskCompletion(TaskID task);
		
	protected:
		void WaitForWork();
		virtual void WorkOnTask(TaskID task, float delta) = 0;
		
	private:
		void WorkerLoop();
		
		float _delta;
		
		TaskID _task;
		TaskID _lastTask;
		
		std::condition_variable _waitCondition;
		std::condition_variable _workCondition;
		
		std::mutex _waitMutex;
		std::mutex _workMutex;
	};
}

#endif /* __RAYNE_PIPELINESEGMENT_H__ */
