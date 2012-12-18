//
//  RNContextOGL.cpp
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNContextOGL.h"

namespace RN
{
	ContextOGL::ContextOGL(ContextFlags flags, ContextOGL *shared)
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
			oglProfile, NSOpenGLProfileVersion3_2Core,
			0
		};
		
		NSOpenGLPixelFormat *pixelFormat = [[[NSOpenGLPixelFormat alloc] initWithAttributes:formatAttributes] autorelease];
		if(!pixelFormat)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsNoHardware);
		
		_oglContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:_shared ? _shared->_oglContext : nil];
		if(!_oglContext)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);
#else
		throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);
#endif
	}
	
	ContextOGL::~ContextOGL()
	{
		if(_shared)
			_shared->Release();
		
#if RN_PLATFORM_MAC_OS
		[_oglContext release];
#endif
	}
	
	void ContextOGL::Flush()
	{
		glFlush();
	}
	
	void ContextOGL::Activate()
	{
#if RN_PLATFORM_MAC_OS
		[_oglContext makeCurrentContext];
#endif
	}
	
	void ContextOGL::Deactivate()
	{
#if RN_PLATFORM_MAC_OS
		[NSOpenGLContext clearCurrentContext];
#endif
	}
}
