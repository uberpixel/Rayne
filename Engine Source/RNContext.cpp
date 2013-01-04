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
	Context::Context(ContextFlags flags, Context *shared)
	{
		_active = false;
		_flags  = flags;
		_thread = 0;
		_shared = shared;
		_shared->Retain();
		
#if RN_PLATFORM_MAC_OS
		_oglPixelFormat = CreatePixelFormat(flags);
		if(!_oglPixelFormat)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsNoHardware);
		
		_oglContext = [[NSOpenGLContext alloc] initWithFormat:_oglPixelFormat shareContext:_shared ? _shared->_oglContext : nil];
		if(!_oglContext)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);
		
		_cglContext = (CGLContextObj)[_oglContext CGLContextObj];
#elif RN_PLATFORM_IOS
		EAGLSharegroup *sharegroup = _shared ? [_shared->_oglContext sharegroup] : nil;
		
		_oglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2 sharegroup:sharegroup];
		if(!_oglContext)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);
#else
		throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);
#endif
	}
	
	Context::Context(Context *shared)
	{
		_active = false;
		_thread = 0;
		_shared = shared;
		_shared->Retain();
		
#if RN_PLATFORM_MAC_OS
		_glsl = shared->_glsl;
		
		_oglPixelFormat = [[NSOpenGLPixelFormat alloc] initWithCGLPixelFormatObj:[_shared->_oglPixelFormat CGLPixelFormatObj]];
		if(!_oglPixelFormat)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsNoHardware);
		
		_oglContext = [[NSOpenGLContext alloc] initWithFormat:_oglPixelFormat shareContext:_shared->_oglContext];
		if(!_oglContext)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);
		
		_cglContext = (CGLContextObj)[_oglContext CGLContextObj];
		
		CGLEnable(_shared->_cglContext, kCGLCEMPEngine);
		CGLEnable(_cglContext, kCGLCEMPEngine);
		
		if(_shared->_active && _shared->_thread->OnThread())
		{
			// Reactivate the context
			[_shared->_oglContext makeCurrentContext];
		}
		
#elif RN_PLATFORM_IOS
		EAGLSharegroup *sharegroup = [_shared->_oglContext sharegroup];
		
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
	}
	
#if RN_PLATFORM_MAC_OS
	NSOpenGLPixelFormat *Context::CreatePixelFormat(ContextFlags flags)
	{
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
			oglProfile, NSOpenGLProfileVersion3_2Core,
			0
		};
		
		return [[NSOpenGLPixelFormat alloc] initWithAttributes:formatAttributes];
	}
#endif
	
	
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
	
	void Context::DeactiveContext()
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
