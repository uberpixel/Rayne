//
//  RNThread.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_THREAD_H__
#define __RAYNE_THREAD_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNArray.h"

namespace RN
{
	class Mutex;
    class Context;
	
	class Texture;
	class Camera;
	class Mesh;
	class Shader;
	
	class Thread : public Object
	{
	public:
		friend class Context;
		friend class Texture;
		friend class Camera;
		friend class Mesh;
		friend class Shader;
		
		typedef void (*ThreadEntry)(Thread *thread);
		
		Thread(ThreadEntry entry);
		virtual ~Thread();
		
		bool OnThread() const;
		void Detach();
		void Exit();
		
		Texture *CurrentTexture() const { return (Texture *)_textures->LastObject(); }
		Camera *CurrentCamera() const { return (Camera *)_cameras->LastObject(); }
		Mesh *CurrentMesh() const { return (Mesh *)_meshes->LastObject(); }
		
		static void Join(Thread *other);
		static Thread *CurrentThread();
		
	private:
		Thread();
		void Initialize();
		
		void PushTexture(Texture *texture);
		void PopTexture() { _textures->RemoveLastObject(); }
		
		void PushCamera(Camera *camera);
		void PopCamera() { _cameras->RemoveLastObject(); }
		
		void PushMesh(Mesh *mesh);
		void PopMesh() { _meshes->RemoveLastObject(); }
		
		static void *Entry(void *object);
		
		bool _detached;
		ThreadEntry _entry;
        
		Mutex   *_mutex;
		Context *_context;
		
		ObjectArray *_textures;
		ObjectArray *_cameras;
		ObjectArray *_meshes;
		
#if RN_PLATFORM_POSIX
		pthread_t _thread;
#endif
#if RN_PLATFORM_WINDOWS
		HANDLE _thread;
		DWORD _id;
#endif
	};
}

#endif /* __RAYNE_THREAD_H__ */
