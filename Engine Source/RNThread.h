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
	class Kernel;
	class Context;
	
	class Thread : public Object
	{
	public:
		friend class Kernel;
		friend class Context;
		
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
		
		Thread(const Thread *) = delete;
		Thread(Deserializer *) = delete;

		RNAPI ~Thread();
		
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
			LockGuard<SpinLock> lock(_dictionaryLock);
			T *object = _dictionary.GetObjectForKey<T>(key);
			
			return object;
		}
		
		void SetObjectForKey(Object *object, Object *key)
		{
			LockGuard<SpinLock> lock(_dictionaryLock);
			_dictionary.SetObjectForKey(object, key->Copy());
		}
		
		void RemoveObjectForKey(Object *key)
		{
			LockGuard<SpinLock> lock(_dictionaryLock);
			_dictionary.RemoveObjectForKey(key);
		}
		
		RNAPI static Thread *GetCurrentThread();
		RNAPI static Thread *GetMainThread();
		
	private:
		Thread();
		
		void Initialize();
		void Entry();
		void Exit();
		void AutoAssignName();
		
		Mutex _mutex;
		
		Context *_context;
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
		
		RNDeclareMeta(Thread)
	};
}

#endif /* __RAYNE_THREAD_H__ */
