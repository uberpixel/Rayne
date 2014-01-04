//
//  RNKernel.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KERNEL_H__
#define __RAYNE_KERNEL_H__

#include "RNBase.h"
#include "RNAutoreleasePool.h"
#include "RNObject.h"
#include "RNApplication.h"
#include "RNRenderer.h"
#include "RNWindow.h"
#include "RNInput.h"
#include "RNUIServer.h"
#include "RNString.h"
#include "RNStatistics.h"

namespace RN
{
#define kRNKernelDidBeginFrameMessage RNCSTR("kRNKernelDidBeginFrameMessage")
#define kRNKernelDidEndFrameMessage   RNCSTR("kRNKernelDidEndFrameMessage")
	
	class World;
	
	class Kernel : public INonConstructingSingleton<Kernel>
	{
	public:
		friend class Settings;
		
		RNAPI Kernel(Application *app);
		RNAPI ~Kernel() override;

		RNAPI bool Tick();

		RNAPI void SetWorld(World *world);
		RNAPI void SetTimeScale(float timeScale);
		RNAPI void SetMaxFPS(uint32 fps);
		
		RNAPI void DidSleepForSignificantTime();
		RNAPI void Exit();
		
		RNAPI void PushStatistics(const std::string& key);
		RNAPI void PopStatistics();
		
		RNAPI const std::vector<Statistics::DataPoint *>& GetStatisticsData() const;

		float GetScaleFactor() const { return _scaleFactor; }
		RNAPI float GetActiveScaleFactor() const;
		
		const std::string& GetTitle() const { return _title; }

		Window *GetWindow() const { return _window; }
		Context *GetContext() const { return _context; }

		float GetDelta() const { return _delta; }
		float GetTime() const { return _time; }
		float GetScaledTime() const { return _scaledTime; }
		float GetTimeScale() const { return _timeScale; }
		
		FrameID GetCurrentFrame() const { return _frame; }

#if RN_PLATFORM_WINDOWS
		HWND GetMainWindow() const { return _mainWindow;  }
		HINSTANCE GetInstance() const { return _instance; }
#endif

	private:
		void Prepare();
		void Initialize();
		void DumpSystem();

#if RN_PLATFORM_POSIX
		void *_appHandle;
#endif
#if RN_PLATFORM_WINDOWS
		HMODULE _appHandle;
#endif
		
		std::string _title;
		FrameID _frame;
		float _scaleFactor;
		
		Application *_app;
		Thread *_mainThread;

		Window *_window;
		Context *_context;
		
		AutoreleasePool *_pool;
		Renderer *_renderer;
		Input *_input;
		World *_world;
		UI::Server *_uiserver;
		
		uint32 _statisticsSwitch;
		Statistics _statistics[2];
		
		uint32 _maxFPS;
		float _minDelta;

		bool _resetDelta;
		bool _shouldExit;
		bool _initialized;

		float _time;
		float _scaledTime;
		float _delta;
		float _timeScale;
		std::chrono::steady_clock::time_point _lastFrame;

#if RN_PLATFORM_WINDOWS
		HWND _mainWindow;
		HINSTANCE _instance;
		WNDCLASSEXW _windowClass;
#endif
		
		RNDefineSingleton(Kernel)
	};
}

#endif /* __RAYNE_KERNEL_H__ */
