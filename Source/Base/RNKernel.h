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

namespace RN
{
	class Kernel
	{
	public:
		Kernel();

		void Run();

	private:
		void Bootstrap();
		void HandleObserver(RunLoopObserver *observer, RunLoopObserver::Activity activity);

		Thread *_mainThread;
		RunLoop *_runLoop;

		RunLoopObserver *_observer;
		bool _exit;
	};
}

#endif /* __RAYME_KERNEL_H___ */
