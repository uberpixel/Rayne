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
	static Kernel *sharedKernel = 0;
	
	Kernel::Kernel()
	{
		_context = new Context::Context();
		_context->MakeActiveContext();
		
		ReadOpenGLExtensions();
		Kernel::CheckOpenGLError("Meh...");
		
		_renderer = new RendererFrontend();
		
		Kernel::CheckOpenGLError("Meh...");
		
		_window  = new Window::Window("Rayne", this);
		_window->SetContext(_context);
		
		_world = 0;
	}	
	
	Kernel::~Kernel()
	{
		if(sharedKernel == this)
			sharedKernel = 0;
		
		_window->Release();
		_world->Release();
	}
	
	void Kernel::Update(float delta)
	{
		_context->MakeActiveContext();
		
		if(_world)
		{
			_renderer->BeginFrame();
			_world->Update(delta);
			_renderer->CommitFrame();
		}
		
		_context->DeactivateContext();
		
		CheckOpenGLError("Kernel::Update()");
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
	
	
	bool Kernel::SupportsExtension(const char *extension)
	{
		std::string extensions((const char *)glGetString(GL_EXTENSIONS));
		return (extensions.rfind(extension) != std::string::npos);
	}
	
	void Kernel::CheckOpenGLError(const char *context)
	{
		GLenum error;
		while((error = glGetError()) != GL_NO_ERROR)
		{
			switch(error)
			{
				case GL_INVALID_ENUM:
					printf("OpenGL Error: GL_INVALID_ENUM. Context: %s\n", context);
					break;
					
				case GL_INVALID_VALUE:
					printf("OpenGL Error: GL_INVALID_VALUE. Context: %s\n", context);
					break;
					
				case GL_INVALID_OPERATION:
					printf("OpenGL Error: GL_INVALID_OPERATION. Context: %s\n", context);
					break;
					
				case GL_INVALID_FRAMEBUFFER_OPERATION:
					printf("OpenGL Error: GL_INVALID_FRAMEBUFFER_OPERATION. Context: %s\n", context);
					break;
					
				case GL_OUT_OF_MEMORY:
					printf("OpenGL Error: GL_OUT_OF_MEMORY. Context: %s\n", context);
					break;
					
#if defined(__gl_h_)
				case GL_STACK_UNDERFLOW:
					printf("OpenGL Error: GL_STACK_UNDERFLOW. Context: %s\n", context);
					break;
					
				case GL_STACK_OVERFLOW:
					printf("OpenGL Error: GL_STACK_OVERFLOW. Context: %s\n", context);
					break;
					
				case GL_TABLE_TOO_LARGE:
					printf("OpenGL Error: GL_TABLE_TOO_LARGE. Context: %s\n", context);
					break;
#endif
					
				default:
					printf("Unknown OpenGL Error: %i. Context: %s\n", error, context);
					break;
			}
			
			fflush(stdout);
		}
	}
	
	
	Kernel *Kernel::SharedInstance()
	{
		if(!sharedKernel)
			sharedKernel = new Kernel();
		
		return sharedKernel;
	}
}
