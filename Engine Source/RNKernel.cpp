//
//  RNKernel.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNKernel.h"
#include "RNWorld.h"
#include "RNOpenGL.h"

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
		_shouldExit = false;
		_scaleFactor = 1.0f;
		
#if RN_PLATFORM_IOS
		_scaleFactor = [[UIScreen mainScreen] scale];
#endif

		_context = new Context();
		_context->MakeActiveContext();
		
		ReadOpenGLExtensions();
		
		_renderer = new RenderingPipeline();
		
		_window = new class Window("Rayne", this);
		_window->SetContext(_context);
		_window->Show();
		
		_world = 0;
		_time  = 0;
		_lastFrame = std::chrono::system_clock::now();
		_resetDelta = true;
	
#if RN_PLATFORM_IOS
		_app = RNApplicationCreate(this);
#else
		LoadApplicationModule(module);
		_app = __ApplicationEntry(this);
#endif
		
		RN_ASSERT(_app, "The game module must respond to RNApplicationCreate() and return a valid RN::Application object!");
	}	
	
	Kernel::~Kernel()
	{
		delete _app;
		_window->Release();
		
		delete _renderer;
		_context->Release();
	}

	void Kernel::LoadApplicationModule(const std::string& module)
	{
#if RN_PLATFORM_MAC_OS
		std::string path = File::PathForName(module);
	
		_appHandle = dlopen(path.c_str(), RTLD_LAZY);
		__ApplicationEntry = (RNApplicationEntryPointer)dlsym(_appHandle, "RNApplicationCreate");
		
		RN_ASSERT(__ApplicationEntry, "The game module must provide an application entry point (RNApplicationCreate())");
#endif
	}
	
	bool Kernel::Tick()
	{
		std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
		
#if RN_PLATFORM_WINDOWS
		MSG	message;
		
		while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}
#endif

		if(_shouldExit)
			return false;
		
		if(_resetDelta)
		{
			_lastFrame = now;
			_resetDelta = false;
		}
		
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - _lastFrame).count();
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastFrame).count();
		
		float delta = seconds + (milliseconds / 1000.0f);
		_time += delta;
		
		if(_world)
		{
			_context->MakeActiveContext();
			
			Application::SharedInstance()->GameUpdate(delta);
			_world->BeginUpdate(delta);
			
			Application::SharedInstance()->WorldUpdate(delta);
			_world->FinishUpdate(delta);
			
			_context->DeactivateContext();			
		}

		_lastFrame = now;
		return true;
	}

	void Kernel::Exit()
	{
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
	
	void Kernel::DidSleepForSignificantTime()
	{
		_resetDelta = true;
	}
}
