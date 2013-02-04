//
//  RNContext.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNContext.h"
#include "RNBaseInternal.h"
#include "RNMutex.h"

#if RN_PLATFORM_WINDOWS
extern void RNRegisterWindow();
#endif

namespace RN
{
	Context::Context(Context *shared) :
		RenderingResource("Context")
	{
		_active = false;
		_thread = 0;
		_shared = shared->Retain<Context>();
		_firstActivation = true;
		
#if RN_PLATFORM_MAC_OS
		static NSOpenGLPixelFormatAttribute formatAttributes[] =
		{
			NSOpenGLPFAMinimumPolicy,
			NSOpenGLPFAAccelerated,
			NSOpenGLPFADoubleBuffer,
			NSOpenGLPFAColorSize, 24,
			NSOpenGLPFAAlphaSize, 8,
			NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
			0
		};
		
		_oglPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:formatAttributes];
		if(!_oglPixelFormat)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsNoHardware);
		
		_oglContext = [[NSOpenGLContext alloc] initWithFormat:(NSOpenGLPixelFormat *)_oglPixelFormat shareContext:_shared ? (NSOpenGLContext *)_shared->_oglContext : nil];
		if(!_oglContext)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);
		
		_cglContext = (CGLContextObj)[(NSOpenGLContext *)_oglContext CGLContextObj];
		
		if(_shared)
		{
			// Enable the multithreaded OpenGL Engine
			
			CGLEnable((CGLContextObj)_shared->_cglContext, kCGLCEMPEngine);
			CGLEnable((CGLContextObj)_cglContext, kCGLCEMPEngine);
			
			if(_shared->_active && shared->_thread->OnThread())
			{
				_shared->Deactivate();
				_shared->Activate();
			}
		}
		
#elif RN_PLATFORM_IOS

		EAGLSharegroup *sharegroup = _shared ? [(EAGLContext *)_shared->_oglContext sharegroup] : nil;
		
		_oglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2 sharegroup:sharegroup];
		if(!_oglContext)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);

#elif RN_PLATFORM_WINDOWS
		RNRegisterWindow();

		_hWnd = CreateOffscreenWindow();
		_hDC  = GetDC(_hWnd);

		PIXELFORMATDESCRIPTOR descriptor;
		memset(&descriptor, 0, sizeof(PIXELFORMATDESCRIPTOR));

		descriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		descriptor.nVersion = 1;
		descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		descriptor.iPixelType = PFD_TYPE_RGBA;
		descriptor.cColorBits = 24;
		descriptor.cAlphaBits = 8;
		descriptor.iLayerType = PFD_MAIN_PLANE;

		_pixelFormat = ChoosePixelFormat(_hDC, &descriptor);
		if(!_pixelFormat)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsNoHardware);

		SetPixelFormat(_hDC, _pixelFormat, &descriptor);

		if(!wglCreateContextAttribsARB)
		{
			// Create a temporary pointer to get the right function pointer
			HGLRC _temp = wglCreateContext(_hDC);
			wglMakeCurrent(_hDC, _temp);

			wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
			if(!wglGetExtensionsStringARB)
				throw ErrorException(kErrorGroupGraphics, 0, kGraphicsNoHardware);

			std::string extensions = std::string((const char *)wglGetExtensionsStringARB(_hDC));

			auto createContext = extensions.find("WGL_ARB_create_context");
			auto coreProfile = extensions.find("WGL_ARB_create_context_profile");

			if(createContext == std::string::npos || coreProfile == std::string::npos)
				throw ErrorException(kErrorGroupGraphics, 0, kGraphicsNoHardware);

			wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
			wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
			wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

			wglMakeCurrent(_hDC, 0);
			wglDeleteContext(_temp);
		}

		
		int attributes[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 2,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};

		_context = wglCreateContextAttribsARB(_hDC, _shared ? _shared->_context : 0, attributes);
		if(!_context)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);

		if(wglSwapIntervalEXT)
		{
			wglMakeCurrent(_hDC, _context);
			wglSwapIntervalEXT(1);
			wglMakeCurrent(_hDC, 0);
		}

		if(_shared)
		{
			if(_shared->_active && shared->_thread->OnThread())
			{
				_shared->Deactivate();
				_shared->Activate();
			}

			wglShareLists(_context, _shared->_context);
		}

#else
		throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);
#endif
	}
	
	Context::~Context()
	{
		DeactivateContext();

		if(_shared)
			_shared->Release();

#if RN_PLATFORM_MAC_OS
		[(id)_oglContext release];
		[(id)_oglPixelFormat release];
#endif
		
#if RN_PLATFORM_IOS
		[(EAGLContext *)_oglContext release];
#endif
	}

#if RN_PLATFORM_WINDOWS
	HWND Context::CreateOffscreenWindow()
	{
		DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION  | WS_SYSMENU | WS_MINIMIZEBOX | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

		HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);
		RECT windowRect;

		windowRect.left = 0;
		windowRect.right = 1024;
		windowRect.top = 0;
		windowRect.bottom = 768;

		AdjustWindowRectEx(&windowRect, dwStyle, false, dwExStyle);

		HWND desktop = GetDesktopWindow();
		RECT desktopRect;

		GetWindowRect(desktop, &desktopRect);


		LONG desktopWidth = desktopRect.right - desktopRect.left;
		LONG desktopHeight = desktopRect.bottom - desktopRect.top;

		LONG width = windowRect.right - windowRect.left;
		LONG height = windowRect.bottom - windowRect.top;

		windowRect.left = (desktopWidth / 2) - (width / 2);
		windowRect.top = (desktopHeight / 2) - (height / 2);

		HWND hWnd = CreateWindowExA(dwExStyle, "RNWindowClass", "", dwStyle, windowRect.left, windowRect.top, width, height, 0, 0, hInstance, 0);
		return hWnd;
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
		if([(EAGLContext *)_oglContext respondsToSelector:@selector(setDebugLabel:)])
			[(EAGLContext *)_oglContext setDebugLabel:[NSString stringWithUTF8String:name]];
#endif
	}
	
	void Context::Activate()
	{
#if RN_PLATFORM_MAC_OS
		[(NSOpenGLContext *)_oglContext makeCurrentContext];
		RN_ASSERT0([NSOpenGLContext currentContext] == _oglContext);		
#endif
		
#if RN_PLATFORM_IOS
		BOOL result = [EAGLContext setCurrentContext:(EAGLContext *)_oglContext];
		RN_ASSERT0(result);
#endif

#if RN_PLATFORM_WINDOWS
		wglMakeCurrent(_hDC, _context);
#endif
		
		if(_firstActivation)
		{
			_firstActivation = false;
			
#if GL_FRAMEBUFFER_SRGB
			glEnable(GL_FRAMEBUFFER_SRGB);
#endif
		}
	}
	
	void Context::Deactivate()
	{
#if RN_PLATFORM_MAC_OS
		[NSOpenGLContext clearCurrentContext];
#endif

#if RN_PLATFORM_IOS
		[EAGLContext setCurrentContext:0];
#endif

#if RN_PLATFORM_WINDOWS
		wglMakeCurrent(_hDC, 0);
#endif
	}
}
