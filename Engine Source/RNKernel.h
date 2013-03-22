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
#include "RNRenderer.h"
#include "RNWindow.h"
#include "RNInput.h"

namespace RN
{
	class World;
	class Kernel : public NonConstructingSingleton<Kernel>
	{
	public:
		RNAPI Kernel();
		RNAPI virtual ~Kernel();

		RNAPI bool Tick();

		RNAPI void SetWorld(World *world);
		RNAPI void SetTimeScale(float timeScale);

		RNAPI void DidSleepForSignificantTime();
		RNAPI void Exit();

		float ScaleFactor() const { return _scaleFactor; }

		Window *Window() const { return _window; }
		Context *Context() const { return _context; }

		float Delta() const { return _delta; }
		float Time() const { return _time; }
		float ScaledTime() const { return _scaledTime; }
		float TimeScale() const { return _timeScale; }

	private:
		void Initialize();
		void LoadApplicationModule(const std::string& module);
		void *_appHandle;

		float _scaleFactor;

		Application *_app;
		Thread *_mainThread;

		class Window *_window;
		class Context *_context;
		
		Renderer *_renderer;
		Input *_input;
		World *_world;

		bool _resetDelta;
		bool _shouldExit;
		bool _initialized;

		float _time;
		float _scaledTime;
		float _delta;
		float _timeScale;
		std::chrono::time_point<std::chrono::steady_clock> _lastFrame;
	};
}

#endif /* __RAYNE_KERNEL_H__ */
