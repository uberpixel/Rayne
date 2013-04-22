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
#include "RNArray.h"
#include "RNSpinLock.h"
#include "RNMesh.h"
#include "RNTexture.h"
#include "RNCamera.h"

namespace RN
{
	class Mutex;
	class Context;
	class Kernel;
	class AutoreleasePool;
	
	class Thread : public Object
	{
	friend class Context;
	friend class Texture;
	friend class Camera;
	friend class Mesh;
	friend class RenderStorage;
	friend class AutoreleasePool;
	friend class Kernel;
	public:
		template<typename F>
		Thread(F&& func, bool detach=true)
		{
			Initialize();
			
			_function = func;
			
			if(detach)
				Detach();
		}
		
		RNAPI virtual ~Thread();
		
		void Detach()
		{
			std::thread thread = std::thread([=]() {
				Entry();
				try
				{
					_function();
				}
				catch (ErrorException e)
				{
					__HandleExcption(e);
				}
				
				Exit();
			});
			
			thread.detach();
		}
		
		RNAPI bool OnThread() const;
		
		RNAPI void Cancel();
		bool IsCancelled() const { return _isCancelled; }
		bool IsRunning() const { return _isRunning; }
		
		RNAPI void SetName(const std::string& name);
		RNAPI const std::string Name() const;
		
		template <typename T>
		T *ObjectForKey(const std::string& key)
		{
			_dictionaryLock.Lock();
			void *object = _dictionary[key];
			_dictionaryLock.Unlock();
			
			return static_cast<T *>(object);
		}
		
		template <typename T>
		void SetObjectForKey(T *object, const std::string& key)
		{
			_dictionaryLock.Lock();
			_dictionary[key] = static_cast<void *>(object);
			_dictionaryLock.Unlock();
		}
		
		RNAPI static Thread *CurrentThread();
		
	private:
		Thread();
		
		void Initialize();
		void Entry();
		void Exit();
		
		void PushTexture(Texture *texture);
		void PopTexture() { _textures->RemoveLastObject(); }
		
		void PushCamera(Camera *camera);
		void PopCamera() { _cameras->RemoveLastObject(); }
		
		void PushMesh(Mesh *mesh);
		void PopMesh() { _meshes->RemoveLastObject(); }
		
		void PushStorage(RenderStorage *storage);
		void PopStorage() { _storages->RemoveLastObject(); }
		
		Texture *CurrentTexture() const { return _textures->LastObject(); }
		Camera *CurrentCamera() const { return _cameras->LastObject(); }
		Mesh *CurrentMesh() const { return _meshes->LastObject(); }
		RenderStorage *CurrentStorage() const { return _storages->LastObject(); }
		
		
		Mutex *_mutex;
		Context *_context;
		AutoreleasePool *_pool;
		SpinLock _dictionaryLock;
		
		Array<Texture> *_textures;
		Array<Camera> *_cameras;
		Array<Mesh> *_meshes;
		Array<RenderStorage> *_storages;
		
		bool _isRunning;
		bool _isCancelled;
		
		std::function<void ()> _function;
		std::thread::id _id;
		
		std::string _name;
		std::map<std::string, void *> _dictionary;
		
		RNDefineConstructorlessMeta(Thread, Object)
	};
}

#endif /* __RAYNE_THREAD_H__ */
