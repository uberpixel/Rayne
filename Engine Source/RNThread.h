//
//  RNThread.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		
		RNAPI void Cancel();
		bool IsCancelled() const { return _isCancelled.load(); }
		bool IsRunning() const { return _isRunning.load(); }
		
		RNAPI void SetName(const std::string& name);
		RNAPI const std::string Name();
		
		RNAPI uint32 SetOpenGLBinding(GLenum target, GLuint object);
		RNAPI GLuint GetOpenGLBinding(GLenum target);
		
		template <typename T>
		T *ObjectForKey(Object *key)
		{
			_dictionaryLock.Lock();
			
			Object *object = _dictionary.ObjectForKey(key);
			if(object)
				object->Retain()->Autorelease();
			
			_dictionaryLock.Unlock();
			
			return object ? object->Downcast<T>() : nullptr;
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
		
		RNAPI static Thread *CurrentThread();
		
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
		
		Function _function;
		std::thread::id _id;
		
		std::string _name;
		Dictionary _dictionary;
		std::unordered_map<GLenum, std::tuple<GLuint, uint32>> _glBindings;
		
		RNDefineMeta(Thread, Object)
	};
}

#endif /* __RAYNE_THREAD_H__ */
