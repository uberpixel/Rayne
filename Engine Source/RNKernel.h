//
//  RNKernel.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KERNEL_H__
#define __RAYNE_KERNEL_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNRenderingResource.h"
#include "RNRendererFrontend.h"
#include "RNWindow.h"

namespace RN
{
	class World;
	class Kernel : public Object, public Singleton<Kernel>
	{
	public:
		Kernel();
		virtual ~Kernel();
		
		void Update(float delta);
		void SetContext(Context *context);
		void SetWorld(World *world);
		
		RendererFrontend *Renderer() const { return _renderer; }
		RendererBackend *RendererBackend() const { return _renderer->Backend(); }
		Window *Window() const { return _window; }
		
		static Kernel *SharedInstance();
		
		static bool SupportsExtension(const char *extension);
		static void CheckOpenGLError(const char *context);
		
	private:		
		class Window *_window;
		Context *_context;
		RendererFrontend *_renderer;
		World *_world;
	};
}

#endif /* __RAYNE_KERNEL_H__ */
