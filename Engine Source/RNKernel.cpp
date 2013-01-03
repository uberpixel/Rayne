//
//  RNKernel.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNKernel.h"

namespace RN
{
	static Kernel *sharedKernel = 0;
	static SpinLock errorLock;
	
	void DrawOffscreenWindows();
	
	Kernel::Kernel()
	{
		_context = new Context::Context(Context::DepthBufferSize8 | Context::StencilBufferSize8);
		_context->MakeActiveContext();
		
		_renderer = new RendererFrontend();
		
		_window  = new Window::Window("Rayne");
		_window->SetContext(_context, _renderer->Backend());
	}	
	
	Kernel::~Kernel()
	{
		if(sharedKernel == this)
			sharedKernel = 0;
		
		delete _window;
	}
	
	
	
	void Kernel::Update(float delta)
	{
		_context->MakeActiveContext();
		DrawOffscreenWindows();
		_context->DeactiveContext();
		
		_renderer->BeginFrame();
		_renderer->CommitFrame();		
	}
	
	void Kernel::SetContext(Context *context)
	{
		_context->Release();
		_context = context;
		_context->Retain();
		
		_window->SetContext(_context, _renderer->Backend());
	}
	
	
	void Kernel::CheckOpenGLError(const char *context)
	{
		//errorLock.Lock();
		
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
					
				case GL_STACK_UNDERFLOW:
					printf("OpenGL Error: GL_STACK_UNDERFLOW. Context: %s\n", context);
					break;
					
				case GL_STACK_OVERFLOW:
					printf("OpenGL Error: GL_STACK_OVERFLOW. Context: %s\n", context);
					break;
					
				default:
					printf("Unknown OpenGL Error: %i. Context: %s\n", error, context);
					break;
			}
			
			fflush(stdout);
		}
		
		//errorLock.Unlock();
	}
	
	
	Kernel *Kernel::SharedInstance()
	{
		if(!sharedKernel)
			sharedKernel = new Kernel();
		
		return sharedKernel;
	}
}
