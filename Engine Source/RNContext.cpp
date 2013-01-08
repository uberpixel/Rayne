//
//  RNContext.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNContext.h"
#include "RNMutex.h"

namespace RN
{
	Context::Context(Context *shared) :
		RenderingResource("Context")
	{
		_active = false;
		_thread = 0;
		_shared = shared;
		_shared->Retain();
		
#if RN_PLATFORM_MAC_OS
		static NSOpenGLPixelFormatAttribute formatAttributes[] =
		{
			NSOpenGLPFAMinimumPolicy,
			NSOpenGLPFAAccelerated,
			NSOpenGLPFADoubleBuffer,
			NSOpenGLPFAColorSize, 24,
			NSOpenGLPFAAlphaSize, 8,
			NSOpenGLPFADepthSize, 24,
			NSOpenGLPFAStencilSize, 8,
			NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
			0
		};
		
		_oglPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:formatAttributes];
		if(!_oglPixelFormat)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsNoHardware);
		
		_oglContext = [[NSOpenGLContext alloc] initWithFormat:_oglPixelFormat shareContext:_shared ? _shared->_oglContext : nil];
		if(!_oglContext)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);
		
		_cglContext = (CGLContextObj)[_oglContext CGLContextObj];
		
		if(_shared)
		{
			// Enable the multithreaded OpenGL Engine
			
			CGLEnable(_shared->_cglContext, kCGLCEMPEngine);
			CGLEnable(_cglContext, kCGLCEMPEngine);
			
			if(_shared->_active && shared->_thread->OnThread())
			{
				_shared->Deactivate();
				_shared->Activate();
			}
		}
		
#elif RN_PLATFORM_IOS
		EAGLSharegroup *sharegroup = _shared ? [_shared->_oglContext sharegroup] : nil;
		
		_oglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2 sharegroup:sharegroup];
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
		
#if RN_PLATFORM_IOS
		[_oglContext release];
#endif
	}
	
	void Context::MakeActiveContext()
	{
		Thread *thread = Thread::CurrentThread();
		RN_ASSERT0(thread);
		
		thread->_mutex->Lock();
		
		if(thread->_context == this)
		{
			thread->_mutex->Unlock();
			return;
		}
		
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
	
	void Context::DeactivateContext()
	{
		Thread *thread = Thread::CurrentThread();
		RN_ASSERT0(thread);
		
		thread->_mutex->Lock();
		
		this->_active = false;
		this->_thread = 0;
		
		this->Deactivate();
		
		thread->_context = 0;
		thread->_mutex->Unlock();
	}
	
	Context *Context::ActiveContext()
	{
		Thread *thread = Thread::CurrentThread();
		RN_ASSERT0(thread);
		
		return thread->_context;
	}
	
	void Context::SetName(const char *name)
	{
		RenderingResource::SetName(name);
		
#if RN_PLATFORM_IOS
		if([_oglContext respondsToSelector:@selector(setDebugLabel:)])
			[_oglContext setDebugLabel:[NSString stringWithUTF8String:name]];
#endif
	}
	
	void Context::Flush()
	{
#if RN_PLATFORM_MAC_OS
		CGLFlushDrawable(_cglContext);
#endif
	}
	
	void Context::Activate()
	{
#if RN_PLATFORM_MAC_OS
		CGLLockContext(_cglContext);
		[_oglContext makeCurrentContext];
		
		RN_ASSERT0([NSOpenGLContext currentContext] == _oglContext);		
#endif
		
#if RN_PLATFORM_IOS
		BOOL result = [EAGLContext setCurrentContext:_oglContext];
		RN_ASSERT0(result);
#endif
	}
	
	void Context::Deactivate()
	{
#if RN_PLATFORM_MAC_OS
		CGLUnlockContext(_cglContext);
#endif
	}
}
