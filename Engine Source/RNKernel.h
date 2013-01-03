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
#include "RNRenderering.h"
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
		
		static Kernel *SharedInstance();
		static void CheckOpenGLError(const char *context);
		
	private:
		Context *_context;
		Window *_window;
		RendererFrontend *_renderer;
	};
}

#endif /* __RAYNE_KERNEL_H__ */
