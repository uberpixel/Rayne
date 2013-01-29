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
#include "RNApplication.h"
#include "RNRenderingResource.h"
#include "RNRenderingPipeline.h"
#include "RNWindow.h"

namespace RN
{
	class World;
	class Kernel : public Object, public UnconstructingSingleton<Kernel>
	{
	public:
		RNAPI Kernel(const std::string& module);
		RNAPI virtual ~Kernel();
		
		RNAPI bool Tick();
		RNAPI void SetContext(Context *context);
		RNAPI void SetWorld(World *world);
		
		RNAPI void DidSleepForSignificantTime();

		RNAPI void Exit();
		
		RenderingPipeline *Renderer() const { return _renderer; }
		Window *Window() const { return _window; }
		
	private:
		void LoadApplicationModule(const std::string& module);
		void *_appHandle;
		
		Application *_app;
		
		class Window *_window;
		Context *_context;
		RenderingPipeline *_renderer;
		World *_world;
		
		bool _resetDelta;
		bool _shouldExit;
		float _time;
		std::chrono::time_point<std::chrono::system_clock> _lastFrame;
	};
}

#endif /* __RAYNE_KERNEL_H__ */
