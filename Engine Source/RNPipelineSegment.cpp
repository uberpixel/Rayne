//
//  RNPipelineSegment.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPipelineSegment.h"

namespace RN
{
	PipelineSegment::PipelineSegment()
	{
		_task = kPipelineSegmentNullTask;
		_lastTask = 0;
		
		_workerLock = new Mutex();
	}
	
	PipelineSegment::~PipelineSegment()
	{
		RN_ASSERT0(1);
	}
	
	
	void PipelineSegment::WaitForWork()
	{
		while(1)
		{
			_workerLock->Lock();
			
			if(_task != kPipelineSegmentNullTask)
			{
				WorkOnTask(_task);
				
				_lastTask = _task;
				_task = kPipelineSegmentNullTask;
				_workerLock->Unlock();
				
				return;
			}
			else
			{
				_workerLock->Unlock();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	}
	
	PipelineSegment::TaskID PipelineSegment::BeginTask()
	{
		_workerLock->Lock();
		
		TaskID ntask = _lastTask + 1;
		_task = ntask;
		
		_workerLock->Unlock();
		
		return ntask;
	}
	
	void PipelineSegment::WaitForTaskCompletion(TaskID task)
	{
		while(_task == task && _task != kPipelineSegmentNullTask)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
