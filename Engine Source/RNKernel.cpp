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
#include "RNApplication.h"

//extern "C" void RNApplicationCreate(RN:Kernel *kernel);

typedef void (*RNApplicationEntryPointer)(RN::Kernel *);

namespace RN
{
	RNApplicationEntryPointer __ApplicationEntry = 0;
	
	Kernel::Kernel(const std::string& module)
	{
		_shouldExit = false;

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
		
		LoadApplicationModule(module);
		
		__ApplicationEntry(this);
		RN_ASSERT0(Application::SharedInstance());
	}	
	
	Kernel::~Kernel()
	{
		_window->Release();
		_world->Release();
		
		delete _renderer;
		_context->Release();
	}

	void Kernel::LoadApplicationModule(const std::string& module)
	{
		std::string path = File::PathForName(module);
		
		_appHandle = dlopen(path.c_str(), RTLD_LAZY);
		__ApplicationEntry = (RNApplicationEntryPointer)dlsym(_appHandle, "RNApplicationCreate");
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
		_world->Release();
		_world = world->Retain<World>();
	}
	
	void Kernel::DidSleepForSignificantTime()
	{
		_resetDelta = true;
	}
}
