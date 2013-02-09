//
//  RNKernel.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNKernel.h"
#include "RNBaseInternal.h"
#include "RNWorld.h"
#include "RNOpenGL.h"
#include "RNAutoreleasePool.h"

#if RN_PLATFORM_IOS
extern "C" RN::Application *RNApplicationCreate(RN::Kernel *);
#else
typedef RN::Application *(*RNApplicationEntryPointer)(RN::Kernel *);
RNApplicationEntryPointer __ApplicationEntry = 0;
#endif

namespace RN
{
	Kernel::Kernel(const std::string& module)
	{
		AutoreleasePool *pool = new AutoreleasePool();

		_context = new Context();
		_context->MakeActiveContext();
		
		ReadOpenGLExtensions();
		
		_scaleFactor = 1.0f;
		
#if RN_PLATFORM_IOS
		_scaleFactor = [[UIScreen mainScreen] scale];
#endif
#if RN_PLATFORM_MAC_OS
		if([[NSScreen mainScreen] respondsToSelector:@selector(backingScaleFactor)])
		{
			_scaleFactor = [NSScreen mainScreen].backingScaleFactor;
		}
#endif
		
		_renderer = new RenderingPipeline();
		_input    = Input::SharedInstance();
		
		_world  = 0;
		_window = 0;
		
		_delta = 0.0f;
		_time  = 0.0f;
		_scaledTime = 0.0f;
		_timeScale  = 1.0f;
		
		_lastFrame  = std::chrono::system_clock::now();
		_resetDelta = true;
		
		_initialized = false;
		_shouldExit  = false;
		
		LoadApplicationModule(module);
		delete pool;
	}	
	
	Kernel::~Kernel()
	{
		AutoreleasePool *pool = new AutoreleasePool();
		
		_app->WillExit();
		_window->Release();
		
		delete _renderer;
		delete _app;
		
		_context->Release();
		
		delete pool;
	}

	void Kernel::LoadApplicationModule(const std::string& module)
	{
#if RN_PLATFORM_MAC_OS
		std::string path = File::PathForName(module);
	
		_appHandle = dlopen(path.c_str(), RTLD_LAZY);
		__ApplicationEntry = (RNApplicationEntryPointer)dlsym(_appHandle, "RNApplicationCreate");
		
		RN_ASSERT(__ApplicationEntry, "The game module must provide an application entry point (RNApplicationCreate())");
#endif
		
		_app = __ApplicationEntry(this);
		RN_ASSERT(_app, "The game module must respond to RNApplicationCreate() and return a valid RN::Application object!");
	}
	
	void Kernel::Initialize()
	{
		if(!_window)
		{
			_window = new class Window("Rayne", this);
			_window->SetContext(_context);
			_window->Show();
		}
		
		_app->Start();
	}
	
	bool Kernel::Tick()
	{
		std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
		AutoreleasePool *pool = new AutoreleasePool();
		
		if(!_initialized)
		{
			Initialize();
			_initialized = true;
		}
		
		if(_resetDelta)
		{
			_lastFrame = now;
			_resetDelta = false;
		}
		
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - _lastFrame).count();
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastFrame).count();
		
		float trueDelta = seconds + (milliseconds / 1000.0f);
		
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
		
		if(_world)
		{
			_context->MakeActiveContext();
			
			Application::SharedInstance()->GameUpdate(_delta);
			_world->BeginUpdate(_delta);
			_input->DispatchInputEvents();
			
			Application::SharedInstance()->WorldUpdate(_delta);
			_world->FinishUpdate(_delta);
			
			_context->DeactivateContext();			
		}

		_lastFrame = now;
		
		delete pool;
		return (_shouldExit == false);
	}

	void Kernel::Exit()
	{
		if(_app->CanExit())
			_shouldExit = true;
	}
	
	
	void Kernel::SetContext(Context *context)
	{
		_context->Release();
		_context = context->Retain<Context>();
		
		_window->SetContext(_context);
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
