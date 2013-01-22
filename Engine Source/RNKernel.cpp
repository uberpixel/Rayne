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

namespace RN
{
	Kernel::Kernel()
	{
		_shouldExit = false;

		_context = new Context();
		_context->MakeActiveContext();
		
		ReadOpenGLExtensions();
		
		_renderer = new RendererFrontend();
		
		_window = new class Window("Rayne", this);
		_window->SetContext(_context);
		_window->Show();
		
		_world = 0;
		_time  = 0;
		_lastFrame = std::chrono::system_clock::now();
	}	
	
	Kernel::~Kernel()
	{
		_window->Release();
		_world->Release();
		
		_renderer->Release();
		_context->Release();
	}

	bool Kernel::Tick()
	{
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

		std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
		
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - _lastFrame).count();
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastFrame).count();
		
		float delta = seconds + (milliseconds / 1000.0f);
		_time += delta;
		
		if(_world)
		{
			_context->MakeActiveContext();
			
			_renderer->BeginFrame();
			_world->Update(delta);
			_renderer->CommitFrame(_time);
			
			RN_CHECKOPENGL();
			_context->DeactivateContext();
		}

		_lastFrame = now;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		return true;
	}

	void Kernel::Exit()
	{
		_shouldExit = true;
	}
	
	void Kernel::SetContext(Context *context)
	{
		_context->Release();
		_context = context;
		_context->Retain();
		
		_window->SetContext(_context);
	}
	
	void Kernel::SetWorld(World *world)
	{
		_world->Release();
		_world = world;
		_world->Retain();
	}
}
