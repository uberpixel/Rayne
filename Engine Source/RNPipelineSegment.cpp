//
//  RNPipelineSegment.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPipelineSegment.h"
#include "RNAutoreleasePool.h"

namespace RN
{
	PipelineSegment::PipelineSegment(bool spinThread)
	{
		_task = kPipelineSegmentNullTask;
		_lastTask = 0;
		
		_thread = 0;
		_thread = spinThread ? new Thread(std::bind(&PipelineSegment::WorkLoop, this)) : 0;
	}
	
	PipelineSegment::~PipelineSegment()
	{
		_thread->Cancel();
		
		while(_thread->IsRunning())
		{}
		
		_thread->Release();
	}
	
	void PipelineSegment::SetThread(Thread *thread)
	{
		RN_ASSERT(!_thread, "The thread of a PipelineSegment can't be replaced!");
		_thread = thread->Retain<Thread>();
	}
	
	void PipelineSegment::WorkLoop()
	{
		while(!_thread)
		{}
		
		while(!_thread->IsCancelled())
			WaitForWork();
	}
	
	void PipelineSegment::WaitForWork()
	{
		std::unique_lock<std::mutex> lock(_workMutex);
		bool result;
		
		do {
			result = _workCondition.wait_for(lock, std::chrono::milliseconds(50), [&](){ return (_task != kPipelineSegmentNullTask); });
		
			if(_thread->IsCancelled())
				return;
		
		} while(!result);
		
		std::lock_guard<std::mutex> waitLock(_waitMutex);
		
		AutoreleasePool *pool = new AutoreleasePool();
		{
			WorkOnTask(_task, _delta);
		}
		delete pool;
		
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
}
