//
//  RNKernel.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNKernel.h"
#include "RNBaseInternal.h"
#include "RNWorld.h"
#include "RNOpenGL.h"
#include "RNAutoreleasePool.h"
#include "RNThreadPool.h"
#include "RNSettings.h"
#include "RNModule.h"
#include "RNPathManager.h"
#include "RNCPU.h"
#include "RNResourcePool.h"

#if RN_PLATFORM_IOS
extern "C" RN::Application *RNApplicationCreate(RN::Kernel *);
#endif

typedef RN::Application *(*RNApplicationEntryPointer)(RN::Kernel *);
RNApplicationEntryPointer __ApplicationEntry = 0;

namespace RN
{
#if RN_PLATFORM_INTEL
	namespace X86_64
	{
		extern void GetCPUInfo();
	}
#endif
	
	namespace Debug
	{
		extern void InstallDebugDraw();
	}
	
	Kernel::Kernel(const std::string& title) :
		_title(title)
	{
		Prepare();
		LoadApplicationModule(Settings::SharedInstance()->ObjectForKey<String>(kRNSettingsGameModuleKey));
	}
	
	Kernel::Kernel(Application *app) :
		_title(app->Title())
	{
		Prepare();
		_app = app;
	}

	Kernel::~Kernel()
	{
		AutoreleasePool *pool = new AutoreleasePool();
		_app->WillExit();
		
		delete _app;

		delete _renderer;
		delete _input;
		delete _uiserver;

#if RN_PLATFORM_LINUX
		Display *dpy = Context::_dpy;
		_context->Release();
		XCloseDisplay(dpy);
#else
		_context->Release();
#endif

		delete pool;
		_mainThread->Exit();
		_mainThread->Release();
	}
	
	void Kernel::Prepare()
	{
#if RN_PLATFORM_LINUX
		XInitThreads();
#endif
		
#if RN_PLATFORM_INTEL
		X86_64::GetCPUInfo();
		X86_64::Capabilities caps = X86_64::Caps();
		
		if(!(caps & X86_64::CAP_SSE) || !(caps & X86_64::CAP_SSE2))
			throw ErrorException(kErrorGroupSystem, kSystemGroupGeneric, kSystemCPUUnsupported);
#endif
		_mainThread = new Thread();
		
		AutoreleasePool *pool = new AutoreleasePool();
		Settings::SharedInstance();
		ThreadCoordinator::SharedInstance();
		
		_context = new class Context();
		_context->MakeActiveContext();
		
		ReadOpenGLExtensions();
		ResourcePool::SharedInstance();
		
		_scaleFactor = 1.0f;
		
		_resourceBatch = ThreadPool::SharedInstance()->OpenBatch();
		_resourceBatch->AddTask([] {
			Debug::InstallDebugDraw();
		});
		
		ResourcePool::SharedInstance()->LoadDefaultResources(_resourceBatch);
		
		_resourceBatch->Commit();
		
#if RN_PLATFORM_IOS
		_scaleFactor = [[UIScreen mainScreen] scale];
#endif
#if RN_PLATFORM_MAC_OS
		if([[NSScreen mainScreen] respondsToSelector:@selector(backingScaleFactor)])
			_scaleFactor = [[NSScreen mainScreen] backingScaleFactor];
#endif
		
		_renderer = Renderer::SharedInstance();
		_input    = Input::SharedInstance();
		_uiserver = UI::Server::SharedInstance();
		
		_world = nullptr;
		_window = Window::SharedInstance();
		_frame  = 0;
		
		_delta = 0.0f;
		_time  = 0.0f;
		_scaledTime = 0.0f;
		_timeScale  = 1.0f;
		
		_lastFrame  = std::chrono::steady_clock::now();
		_resetDelta = true;
		
		_initialized = false;
		_shouldExit  = false;
		
		ModuleCoordinator::SharedInstance();
		delete pool;
	}	

	void Kernel::LoadApplicationModule(String *module)
	{
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_LINUX
		std::string moduleName = std::string(module->UTF8String());
		
#if RN_PLATFORM_MAC_OS
		moduleName += ".dylib";
#endif
		
#if RN_PLATFORM_LINUX
		moduleName += ".so";
#endif
		
		std::string path = PathManager::PathForName(moduleName);

		_appHandle = dlopen(path.c_str(), RTLD_LAZY);
		if(!_appHandle)
			throw ErrorException(0, 0, 0, std::string(dlerror()));
		
		__ApplicationEntry = (RNApplicationEntryPointer)dlsym(_appHandle, "RNApplicationCreate");

		RN_ASSERT(__ApplicationEntry, "The game module must provide an application entry point (RNApplicationCreate())");
#endif
#if RN_PLATFORM_IOS
		__ApplicationEntry = RNApplicationCreate;
#endif

		_app = __ApplicationEntry(this);
		RN_ASSERT(_app, "The game module must respond to RNApplicationCreate() and return a valid RN::Application object!");
	}

	void Kernel::Initialize()
	{
		_resourceBatch->Wait();
		_resourceBatch.reset();
		
		_app->Start();
		_window->SetTitle(_app->Title());
	}

	
	
	bool Kernel::Tick()
	{
		std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
		AutoreleasePool *pool = new AutoreleasePool();

		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastFrame).count();
		float trueDelta = milliseconds / 1000.0f;
		
		if(_resetDelta)
		{
			trueDelta = 0.0f;
			_resetDelta = false;
		}
		
		if(!_initialized)
		{
			Initialize();
			trueDelta = 0.0f;
			
			_initialized = true;
			_resetDelta  = true;
		}

		_time += trueDelta;

		_delta = trueDelta * _timeScale;
		_scaledTime += _delta;

#if RN_PLATFORM_WINDOWS
		MSG	message;

		while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}
#endif
		
#if RN_PLATFORM_LINUX
		XEvent event;
		Atom wmDeleteMessage = XInternAtom(_context->_dpy, "WM_DELETE_WINDOW", False);
		while(XPending(_context->_dpy))
		{
			XNextEvent(_context->_dpy, &event);
			
			switch(event.type)
			{
				case ConfigureNotify:
				case Expose:
				case ClientMessage:
					if(event.xclient.data.l[0] == wmDeleteMessage)
						Exit();
					break;
					
				default:
					_input->HandleEvent(&event);
					break;
			}
		}
#endif
		
		_frame ++;
		_renderer->BeginFrame(_delta);
		_input->DispatchInputEvents();
		
		Application::SharedInstance()->GameUpdate(_delta);

		if(_world)
		{
			_world->StepWorld(_frame, _delta);
			Application::SharedInstance()->WorldUpdate(_delta);
		}
		
		_uiserver->Render(_renderer);
		_renderer->FinishFrame();
		_input->InvalidateFrame();
		
#if RN_PLATFORM_MAC_OS
		CGLFlushDrawable((CGLContextObj)[(NSOpenGLContext *)_context->_oglContext CGLContextObj]);
#endif
		
#if RN_PLATFORM_LINUX
		glXSwapBuffers(_context->_dpy, _context->_win);
#endif
		
		
		static float totalTime = 0.0f;
		static int count = 0;
		
		totalTime += _delta;
		count ++;
		
		if(totalTime >= 1.0f)
		{
			float average = totalTime / count;
			
			printf("Drew %i frames in the last %f seconds. Average frame time: %f\n", count, totalTime, average);
			
			totalTime = 0.0f;
			count = 0;
		}
		
		_lastFrame = now;

		delete pool;
		return (_shouldExit == false);
	}

	void Kernel::Exit()
	{
		_shouldExit = true;
	}


	void Kernel::SetWorld(World *world)
	{
		_world = world;
	}

	void Kernel::SetTimeScale(float timeScale)
	{
		_timeScale = timeScale;
	}

	void Kernel::DidSleepForSignificantTime()
	{
		_resetDelta = true;
		_delta = 0.0f;
	}
}
