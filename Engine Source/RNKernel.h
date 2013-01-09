//
//  RNKernel.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
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
	class Kernel : public Object, public Singleton<Kernel>
	{
	public:
		Kernel();
		virtual ~Kernel();
		
		void Update(float delta);
		void SetContext(Context *context);
		
		RendererFrontend *Renderer() { return _renderer; }
		RendererBackend *RendererBackend() { return _renderer->Backend(); }
		
		static Kernel *SharedInstance();
		
		static bool SupportsExtension(const char *extension);
		static void CheckOpenGLError(const char *context);
		
	private:
		Mesh *mesh;
		Texture *texture;
		Material *material;
		Shader *shader;
		Matrix transform;
		
		Context *_context;
		Window *_window;
		RendererFrontend *_renderer;
	};
}

#endif /* __RAYNE_KERNEL_H__ */
