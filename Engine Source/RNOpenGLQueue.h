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

namespace RN
{
	class Thread;
	class Context;
	
	class OpenGLQueue : public ISingleton<OpenGLQueue>
	{
	public:
		RNAPI OpenGLQueue();
		RNAPI ~OpenGLQueue();
		
		RNAPI void Wait();
		RNAPI void SwitchContext(Context *context);
		
		template<class F>
		std::future<void> SubmitCommand(F &&f, bool wait = false)
		{
			std::packaged_task<void ()> task(std::move(f));
			std::future<void> future = SubmitCommand(std::move(task));
			
			if(wait)
				future.wait();
			
			return future;
		}
		
	private:
		RNAPI std::future<void> SubmitCommand(std::packaged_task<void ()> &&command);
		
		void ProcessCommands();
		void Wait(size_t command);
		
		Thread *_thread;
		Context *_context;
		
		SpinLock _commandLock;
		stl::lock_free_ring_buffer<std::packaged_task<void ()>, 128> _commands;
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
