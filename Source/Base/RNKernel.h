//
//  RNKernel.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KERNEL_H__
#define __RAYNE_KERNEL_H__

#include "RNBase.h"
#include "../Threads/RNThread.h"
#include "../Threads/RNRunLoop.h"
#include "../Threads/RNWorkQueue.h"

namespace RN
{
	class Kernel
	{
	public:
		Kernel();

		void Run();
		void Exit();

	private:
		void Bootstrap();
		void HandleObserver(RunLoopObserver *observer, RunLoopObserver::Activity activity);

		Thread *_mainThread;
		RunLoop *_runLoop;
		WorkQueue *_mainQueue;

		RunLoopObserver *_observer;
		std::atomic<bool> _exit;
	};
}

#endif /* __RAYME_KERNEL_H___ */
