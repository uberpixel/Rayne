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
		
		virtual void WorkOnTask(TaskID task) = 0;
		
		void BeginTask(TaskID task);
		void WaitForTaskCompletion(TaskID task);
		
	private:
		void WorkerLoop();
		
		TaskID _task;
		Thread *_thread;
		Mutex  *_workerLock;
	};
}

#endif /* __RAYNE_PIPELINESEGMENT_H__ */
