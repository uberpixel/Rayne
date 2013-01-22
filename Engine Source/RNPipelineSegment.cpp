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
		_workerLock = new Mutex();
		_thread = 0;
		
		std::thread thread = std::thread(&PipelineSegment::WorkerLoop, this);
		thread.detach();
	}
	
	PipelineSegment::~PipelineSegment()
	{
		RN_ASSERT0(1);
	}
	
	

	
	void PipelineSegment::WorkerLoop()
	{
		_thread = Thread::CurrentThread();
		
		while(1)
		{
			_workerLock->Lock();
			
			if(_task)
			{
				WorkOnTask(_task);
				_task = kPipelineSegmentNullTask;
				
				_workerLock->Unlock();
			}
			else
			{
				_workerLock->Unlock();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		
		_thread->Exit();
	}
	
	
	void PipelineSegment::BeginTask(TaskID task)
	{
		_workerLock->Lock();
		_task = task;
		_workerLock->Unlock();
	}
	
	void PipelineSegment::WaitForTaskCompletion(TaskID task)
	{
		while(_task == task && _task != kPipelineSegmentNullTask)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
