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
	}
	
	PipelineSegment::~PipelineSegment()
	{
		RN_ASSERT0(1);
	}
	
	
	void PipelineSegment::WaitForWork()
	{
		std::unique_lock<std::mutex> lock(_workMutex);		
		_workCondition.wait(lock, [&](){ return _task != kPipelineSegmentNullTask; });
		
		std::lock_guard<std::mutex> waitLock(_waitMutex);
		
		WorkOnTask(_task);
		
		_lastTask = _task;
		_task = kPipelineSegmentNullTask;
		
		_waitCondition.notify_all();
	}
	
	PipelineSegment::TaskID PipelineSegment::BeginTask()
	{
		std::lock_guard<std::mutex> waitLock(_workMutex);
		
		TaskID ntask = _lastTask + 1;
		_task = ntask;
		
		_workCondition.notify_one();
		
		return ntask;
	}
	
	void PipelineSegment::WaitForTaskCompletion(TaskID task)
	{
		std::unique_lock<std::mutex> lock(_waitMutex);
		_waitCondition.wait(lock, [&]() { return (_task != task || _task == kPipelineSegmentNullTask); });
	}
}
