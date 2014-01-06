//
//  RNThread.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_THREAD_H__
#define __RAYNE_THREAD_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNMutex.h"
#include "RNFunction.h"
#include "RNDictionary.h"

namespace RN
{
	class Context;
	class Kernel;
	class AutoreleasePool;
	
	class Thread : public Object
	{
	friend class Context;
	friend class AutoreleasePool;
	friend class Kernel;
	public:
		template<typename F>
		explicit Thread(F&& func, bool detach=true) :
			_function(std::move(func))
		{
			Initialize();
			AutoAssignName();
			
			if(detach)
				Detach();
		}
		
		Thread(Function&& func, bool detach=true) :
			_function(std::move(func))
		{
			Initialize();
			AutoAssignName();
			
			if(detach)
				Detach();
		}

		RNAPI virtual ~Thread();
		
		RNAPI void Detach();
		RNAPI bool OnThread() const;
		RNAPI void WaitForExit();
		
		RNAPI void Cancel();
		RNAPI bool IsCancelled() const { return _isCancelled.load(); }
		RNAPI bool IsRunning() const { return _isRunning.load(); }
		
		RNAPI void SetName(const std::string& name);
		RNAPI const std::string GetName();

		template <typename T>
		T *GetObjectForKey(Object *key)
		{
			_dictionaryLock.Lock();
			
			T *object = _dictionary.GetObjectForKey<T>(key);

			_dictionaryLock.Unlock();
			
			return object;
		}
		
		void SetObjectForKey(Object *object, Object *key)
		{
			_dictionaryLock.Lock();
			_dictionary.SetObjectForKey(object, key);
			_dictionaryLock.Unlock();
		}
		
		void RemoveObjectForKey(Object *key)
		{
			_dictionaryLock.Lock();
			_dictionary.RemoveObjectForKey(key);
			_dictionaryLock.Unlock();
		}
		
		RNAPI static Thread *GetCurrentThread();
		
	private:
		Thread();
		
		void Initialize();
		void Entry();
		void Exit();
		void AutoAssignName();
		
		Mutex _mutex;
		
		Context *_context;
		AutoreleasePool *_pool;
		SpinLock _dictionaryLock;
		
		std::atomic<bool> _isRunning;
		std::atomic<bool> _isCancelled;
		std::atomic<bool> _isDetached;
		
		Function _function;
		std::thread::id _id;
		
		std::string _name;
		Dictionary _dictionary;
		
		std::mutex _exitMutex;
		std::condition_variable _exitSignal;
		
		RNDefineMeta(Thread, Object)
	};
}

#endif /* __RAYNE_THREAD_H__ */
