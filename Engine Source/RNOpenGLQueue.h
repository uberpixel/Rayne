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

#if RN_PLATFORM_WINDOWS && _MSC_VER <= 1800
// Yeah, so... MSVC ships with a buggy thread library... packaged_task<void> doesn't work
// not in VS 2012 nor VS2013...
// See this discussion and source for the below source code: http://stackoverflow.com/questions/14744588/why-is-stdpackaged-taskvoid-not-valid
// We need packaged_task<void> so yeah, fuck it, we are doing this and shipping it and ya'll have Microsoft to blame
namespace std
{
	template<class... _ArgTypes>
	class packaged_task<void(_ArgTypes...)>
	{
		promise<void> _my_promise;
		function<void(_ArgTypes...)> _my_func;
		
	public:
		packaged_task() {
		}
		
		template<class _Fty2>
		explicit packaged_task(_Fty2&& _Fnarg)
		: _my_func(_Fnarg) {
		}
		
		packaged_task(packaged_task&& _Other)
		: _my_promise(move(_Other._my_promise)),
        _my_func(move(_Other._my_func)) {
		}
		
		packaged_task& operator=(packaged_task&& _Other) {
			_my_promise = move(_Other._my_promise);
			_my_func = move(_Other._my_func);
			return (*this);
		}
		
		packaged_task(const packaged_task&) = delete;
		packaged_task& operator=(const packaged_task&) = delete;
		
		~packaged_task() {
		}
		
		void swap(packaged_task& _Other) {
			_my_promise.swap(_Other._my_promise);
			_my_func.swap(_Other._my_func);
		}
		
		explicit operator bool() const {
			return _my_func != false;
		}
		
		bool valid() const {
			return _my_func != false;
		}
		
		future<void> get_future() {
			return _my_promise.get_future();
		}
		
		void operator()(_ArgTypes... _Args) {
			_my_func(forward<_ArgTypes>(_Args)...);
			_my_promise.set_value();
		}
		
		void reset() {
			swap(packaged_task());
		}
	};
}
#endif

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
