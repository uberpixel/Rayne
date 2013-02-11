//
//  RNThread.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_THREAD_H__
#define __RAYNE_THREAD_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNArray.h"

#include "RNMesh.h"
#include "RNTexture.h"
#include "RNCamera.h"

namespace RN
{
	class Mutex;
	class Context;
	
	class Shader;
	class AutoreleasePool;
	
	class Thread : public Object
	{
	friend class Context;
	friend class Texture;
	friend class Camera;
	friend class Mesh;
	friend class Shader;
	friend class AutoreleasePool;
		
	public:
		typedef void (*ThreadEntry)(Thread *thread);
		
		Thread(ThreadEntry entry);
		virtual ~Thread();
		
		bool OnThread() const;
		void Detach();
		void Exit();
		
		Texture *CurrentTexture() const { return _textures->LastObject(); }
		Camera *CurrentCamera() const { return _cameras->LastObject(); }
		Mesh *CurrentMesh() const { return _meshes->LastObject(); }
		
		static Thread *CurrentThread();
		
	private:
		Thread();
		void Initialize();
		void Entry();
		
		void PushTexture(Texture *texture);
		void PopTexture() { _textures->RemoveLastObject(); }
		
		void PushCamera(Camera *camera);
		void PopCamera() { _cameras->RemoveLastObject(); }
		
		void PushMesh(Mesh *mesh);
		void PopMesh() { _meshes->RemoveLastObject(); }
		
		bool _detached;
		ThreadEntry _entry;
		
		Mutex   *_mutex;
		Context *_context;
		AutoreleasePool *_pool;
		
		Array<Texture> *_textures;
		Array<Camera> *_cameras;
		Array<Mesh> *_meshes;
		
		std::thread::id _id;
	};
}

#endif /* __RAYNE_THREAD_H__ */
