//
//  RNThread.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_THREAD_H__
#define __RAYNE_THREAD_H__

#include "../Base/RNBase.h"
#include "../Base/RNFunction.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNDictionary.h"
#include "../Objects/RNString.h"
#include "RNRunLoop.h"

namespace RN
{
	class Kernel;
	
	class Thread : public Object
	{
	public:
		friend class Kernel;
		
		template<typename F>
		explicit Thread(F &&func, bool start=true) :
			_function(std::move(func))
		{
			Initialize();
			AutoAssignName();
			
			if(start)
				Start();
		}
		
		Thread(Function &&func, bool start=true) :
			_function(std::move(func))
		{
			Initialize();
			AutoAssignName();
			
			if(start)
				Start();
		}

		RNAPI ~Thread();

		RNAPI void SetName(const String *name);

		RNAPI void Start();
		RNAPI bool OnThread() const;
		RNAPI void WaitForExit();
		RNAPI void ExecuteOnExit(std::function<void (void *)> &&function, void *context);
		RNAPI void UnscheduleExecuteOnExit(void *context);
		
		RNAPI void Cancel();
		bool IsCancelled() const { return _isCancelled.load(); }
		bool IsRunning() const { return _isRunning.load(); }

		RNAPI String *GetName();
		RunLoop *GetRunLoop() const { return _runLoop; }

		template <typename T>
		T *GetObjectForKey(Object *key)
		{
			LockGuard<Lockable> lock(_dictionaryLock);
			T *object = _dictionary->GetObjectForKey<T>(key);
			
			return object;
		}
		
		void SetObjectForKey(Object *object, Object *key)
		{
			LockGuard<Lockable> lock(_dictionaryLock);
			Object *keyCopy = key->Copy();
			_dictionary->SetObjectForKey(object, keyCopy);
			keyCopy->Release();
		}
		
		void RemoveObjectForKey(Object *key)
		{
			LockGuard<Lockable> lock(_dictionaryLock);
			_dictionary->RemoveObjectForKey(key);
		}
		
		RNAPI static Thread *GetCurrentThread();
		RNAPI static Thread *GetMainThread();
		RNAPI static void CleanUp(); // Must be called on exit for custom spawned threads. RNThreads call this automatically
		
	private:
		Thread();
		
		void Initialize();
		void Entry();
		void Exit();
		void AutoAssignName();
		void __UnscheduleExecuteOnExit(void *context);
		
		Lockable _generalMutex;
		RunLoop *_runLoop;

		Lockable _dictionaryLock;
		Dictionary *_dictionary;
		String *_name;
		
		std::atomic<bool> _isRunning;
		std::atomic<bool> _isCancelled;
		std::atomic<bool> _isDetached;

		Function _function;
		std::thread::id _id;
		std::thread _thread;

		Lockable _exitMutex;
		Condition _exitSignal;
		std::vector<std::pair<std::function<void (void *)>, void *>> _exitFunctions;
		
		__RNDeclareMetaInternal(Thread)
	};
	
	RNObjectClass(Thread)
}

#endif /* __RAYNE_THREAD_H__ */
