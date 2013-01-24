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
		
		_shouldStop = false;
		_didStop = false;
	}
	
	PipelineSegment::~PipelineSegment()
	{
		_shouldStop = true;
		
		while(!_didStop)
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	
	
	void PipelineSegment::WaitForWork()
	{
		std::unique_lock<std::mutex> lock(_workMutex);		
		_workCondition.wait(lock, [&](){ return _task != kPipelineSegmentNullTask; });
		
		std::lock_guard<std::mutex> waitLock(_waitMutex);
		
		WorkOnTask(_task, _delta);
		
		_lastTask = _task;
		_task = kPipelineSegmentNullTask;
		
		_waitCondition.notify_all();
	}
	
	PipelineSegment::TaskID PipelineSegment::BeginTask(float delta)
	{
		std::lock_guard<std::mutex> waitLock(_workMutex);
		
		TaskID ntask = _lastTask + 1;
		_task = ntask;
		_delta = delta;
		
		_workCondition.notify_one();
		
		return ntask;
	}
	
	void PipelineSegment::WaitForTaskCompletion(TaskID task)
	{
		std::unique_lock<std::mutex> lock(_waitMutex);
		_waitCondition.wait(lock, [&]() { return (_task != task || _task == kPipelineSegmentNullTask); });
	}
	
	bool PipelineSegment::ShouldStop()
	{
		return _shouldStop;
	}
	
	void PipelineSegment::DidStop()
	{
		_didStop = true;
	}
}
