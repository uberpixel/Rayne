//
//  RNOpenGLQueue.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENGLQUEUE_H__
#define __RAYNE_OPENGLQUEUE_H__

#include "RNBase.h"
#include "RNRingbuffer.h"
#include "RNSyncPoint.h"

namespace RN
{
	class Thread;
	class Context;
	
	class OpenGLQueue : public ISingleton<OpenGLQueue>
	{
	private:
		struct Task
		{
			Task()
			{}
			
			template<class F>
			Task(F &&f) :
				task(std::move(f))
			{}
			
			Task(Task &&other) :
				syncable(std::move(other.syncable)),
				task(std::move(other.task))
			{}
			
			Task &operator =(Task &&other)
			{
				syncable = std::move(other.syncable);
				task = std::move(other.task);
				
				return *this;
			}
			
			stl::syncable syncable;
			std::function<void ()> task;
		};
		
	public:
		RNAPI OpenGLQueue();
		RNAPI ~OpenGLQueue();
		
		RNAPI void Wait();
		RNAPI void SwitchContext(Context *context);
		
		template<class F>
		stl::sync_point SubmitCommand(F &&f, bool wait = false)
		{
			Task task(std::move(f));
			stl::sync_point point = SubmitCommand(std::move(task));
			
			if(wait)
				point.wait();
			
			return point;
		}
		
	private:
		RNAPI stl::sync_point SubmitCommand(Task &&command);
		void RunTask(Task &task);
		
		void ProcessCommands();
		void Wait(size_t command);
		
		Thread *_thread;
		Context *_context;
		
		SpinLock _commandLock;
		stl::lock_free_ring_buffer<Task, 128> _commands;
		std::atomic<size_t> _processed;
		
		std::atomic<bool> _running;
		std::atomic<size_t> _id;
		std::condition_variable _signal;
		std::condition_variable _waitSignal;
		std::mutex _signalLock;
		
		RNDefineSingleton(OpenGLQueue)
	};
}

#endif /* __RAYNE_OPENGLQUEUE_H__ */
