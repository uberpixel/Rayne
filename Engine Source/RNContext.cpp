//
//  RNContext.cpp
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNContext.h"
#include "RNMutex.h"

namespace RN
{
	Context::Context(ContextFlags flags, Context *shared)
	{
		_shared = shared;
		
#if RN_PLATFORM_MAC_OS
		int32 major;
		int32 minor;
		RN::OSXVersion(&major, &minor, 0);
		
		int32 depthBufferSize   = 0;
		int32 stencilBufferSize = 0;
		
		NSOpenGLPixelFormatAttribute oglProfile = (major == 10 && minor < 7) ? (NSOpenGLPixelFormatAttribute)0 : NSOpenGLPFAOpenGLProfile;
		
		RN::OSXVersion(&major, &minor, 0);
		if(major == 10 && minor < 7)
		{
			oglProfile = (NSOpenGLPixelFormatAttribute)0;
			_glsl = 120;
		}
		else
		{
			_glsl = 150;
		}
		
		depthBufferSize = (flags & DepthBufferSize8) ? 8 : (flags & DepthBufferSize16) ? 16 : (flags & DepthBufferSize24) ? 24 : 0;
		stencilBufferSize = (flags & StencilBufferSize8) ? 8 : (flags & StencilBufferSize16) ? 16 : (flags & StencilBufferSize24) ? 24 : 0;
		
		
		static NSOpenGLPixelFormatAttribute formatAttributes[] =
		{
			NSOpenGLPFAMinimumPolicy,
			NSOpenGLPFAAccelerated,
			NSOpenGLPFADoubleBuffer,
			NSOpenGLPFAColorSize, 24,
			NSOpenGLPFAAlphaSize, 8,
			NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)depthBufferSize,
			NSOpenGLPFAStencilSize, (NSOpenGLPixelFormatAttribute)stencilBufferSize,
			oglProfile, NSOpenGLProfileVersionLegacy,
			0
		};
		
		_oglPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:formatAttributes];
		if(!_oglPixelFormat)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsNoHardware);
		
		_oglContext = [[NSOpenGLContext alloc] initWithFormat:_oglPixelFormat shareContext:_shared ? _shared->_oglContext : nil];
		if(!_oglContext)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);
#else
		throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);
#endif
	}
	
	Context::~Context()
	{
		if(_shared)
			_shared->Release();
		
#if RN_PLATFORM_MAC_OS
		[_oglContext release];
		[_oglPixelFormat release];
#endif
	}
	
	
	void Context::MakeActiveContext()
	{
		Thread *thread = Thread::CurrentThread();
		RN::Assert(thread);
		
		thread->_mutex->Lock();
		
		if(thread->_context)
		{
			Context *other = thread->_context;
			other->_active = false;
			other->_thread = 0;
			
			other->Flush();
			other->Deactivate();
		}
		
		this->Activate();
		
		this->_active = true;
		this->_thread = thread;
		
		thread->_context = this;
		thread->_mutex->Unlock();
	}
	
	void Context::DeactiveContext()
	{
		Thread *thread = Thread::CurrentThread();
		RN::Assert(thread);
		
		thread->_mutex->Lock();
		
		this->_active = false;
		this->_thread = 0;
		
		this->Flush();
		this->Deactivate();
		
		thread->_context = 0;
		thread->_mutex->Unlock();
	}
	
	Context *Context::ActiveContext()
	{
		Thread *thread = Thread::CurrentThread();
		RN::Assert(thread);
		
		return thread->_context;
	}
	
	void Context::Flush()
	{
		glFlush();
	}
	
	void Context::Activate()
	{
#if RN_PLATFORM_MAC_OS
		[_oglContext makeCurrentContext];
#endif
	}
	
	void Context::Deactivate()
	{
#if RN_PLATFORM_MAC_OS
		[NSOpenGLContext clearCurrentContext];
#endif
	}
}
