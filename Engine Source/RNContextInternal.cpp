//
//  RNContextInternal.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNContextInternal.h"
#include "RNBaseInternal.h"
#include "RNMutex.h"
#include "RNSettings.h"
#include "RNKernel.h"

namespace RN
{
	RNDeclareMeta(Context)
	
#if RN_PLATFORM_LINUX
	Display *Context::_dpy = 0;
#endif

	extern void BindOpenGLCore();
	extern void BindOpenGLFunctions(gl::Version version);
	
	bool OpenGLValidateVersion(gl::Version wanted)
	{
		GLint major, minor;

		gl::GetIntegerv(GL_MAJOR_VERSION, &major);
		gl::GetIntegerv(GL_MINOR_VERSION, &minor);

		switch(wanted)
		{
			case gl::Version::Core3_2:
				return ((major == 3 && minor >= 2) || (major > 3));
			
			case gl::Version::Core4_1:
				return ((major == 4 && minor >= 1) || (major > 4));
		}

		return true;
	}

#if RN_PLATFORM_MAC_OS
	void CreateOpenGLContext(gl::Version version, NSOpenGLContext **outContext, NSOpenGLPixelFormat **outFormat)
	{
		RN_ASSERT(outContext, "");
		RN_ASSERT(outFormat, "");
		
		*outContext = nil;
		*outFormat  = nil;
		
		// Sid:
		// Mac OS X doesn't have an extra profile enum for OpenGL 4.1, since it's compatible with 3.2
		// That means, requesting a 3.2 profile on Mavericks will give you a 4.1 context if your
		// hardware supports it. So we create the context, activate it, and then check its version
		// wether it is 3.2 or 4.1 and then check it against the passed version. I know...
		
		static NSOpenGLPixelFormatAttribute formatAttributes[] =
		{
			NSOpenGLPFAClosestPolicy,
			NSOpenGLPFAAccelerated,
			NSOpenGLPFADoubleBuffer,
			NSOpenGLPFAColorSize, 24,
			NSOpenGLPFAAlphaSize, 8,
			NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
			0
		};
		
		NSOpenGLPixelFormat *format = [[NSOpenGLPixelFormat alloc] initWithAttributes:formatAttributes];
		if(!format)
			throw Exception(Exception::Type::NoGPUException, "Couldn't create OpenGL context!");
		
		NSOpenGLContext *context = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
		if(!context)
			throw Exception(Exception::Type::NoGPUException, "Couldn't create OpenGL context!");
		
		// Check the OpenGL contexts version, whatever it reports, it is at least 3.2 core!
		// But a 3.2 context is not a 4.1 context, so we check for that
		NSOpenGLContext *current = [NSOpenGLContext currentContext];
		[context makeCurrentContext];
		
		static std::once_flag flag;
		std::call_once(flag, []{
			gl::GetString = reinterpret_cast<PFNGLGETSTRINGPROC>(dlsym(RTLD_NEXT, "glGetString"));
			
			gl::GetFloatv = reinterpret_cast<PFNGLGETFLOATVPROC>(dlsym(RTLD_NEXT, "glGetFloatv"));
			gl::GetIntegerv = reinterpret_cast<PFNGLGETINTEGERVPROC>(dlsym(RTLD_NEXT, "glGetIntegerv"));
		});
		
		if(!OpenGLValidateVersion(version))
		{
			current ? [current makeCurrentContext] : [NSOpenGLContext clearCurrentContext];
			throw Exception(Exception::Type::NoGPUException, "Couldn't create OpenGL context!");
		}

		current ? [current makeCurrentContext] : [NSOpenGLContext clearCurrentContext];
		
		*outContext = context;
		*outFormat  = format;
	}
#endif
	
#if RN_PLATFORM_WINDOWS
	void CreateOpenGLContext(gl::Version version, HDC hDC, HGLRC shared, HGLRC *outContext)
	{
		int attributes[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 0,
			WGL_CONTEXT_MINOR_VERSION_ARB, 0,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};
		
		switch(version)
		{
			case gl::Version::Core4_1:
				attributes[1] = 4;
				attributes[3] = 1;
				break;
				
			case gl::Version::Core3_2:
				attributes[1] = 3;
				attributes[3] = 2;
				break;
		}
		
		
		HGLRC context = wgl::CreateContextAttribsARB(hDC, shared, attributes);
		if(!context || !wglMakeCurrent(hDC, context))
			throw Exception(Exception::Type::NoGPUException, "Couldn't create OpenGL context!");

		if(!OpenGLValidateVersion(version))
		{
			wglMakeCurrent(hDC, nullptr);
			throw Exception(Exception::Type::NoGPUException, "Couldn't create OpenGL context!");
		}

		wglMakeCurrent(hDC, nullptr);

		*outContext = context;
	}
#endif
	
	
	Context::Context(gl::Version version)
	{
		Initialize(version);
		
#if RN_PLATFORM_MAC_OS
		CreateOpenGLContext(version, &_internals->context, &_internals->pixelFormat);
		
		_internals->cglContext = static_cast<CGLContextObj>([_internals->context CGLContextObj]);
#endif
		
#if RN_PLATFORM_WINDOWS
		_internals->hWnd = Kernel::GetSharedInstance()->GetMainWindow();
		_internals->hDC  = ::GetDC(_internals->hWnd);
		_internals->ownsWindow = false;
		
		PIXELFORMATDESCRIPTOR descriptor;
		memset(&descriptor, 0, sizeof(PIXELFORMATDESCRIPTOR));
		
		descriptor.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
		descriptor.nVersion   = 1;
		descriptor.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		descriptor.iPixelType = PFD_TYPE_RGBA;
		descriptor.cColorBits = 24;
		descriptor.cAlphaBits = 8;
		descriptor.iLayerType = PFD_MAIN_PLANE;
		
		_internals->pixelFormat = ::ChoosePixelFormat(_internals->hDC, &descriptor);
		if(!_internals->pixelFormat)
			throw Exception(Exception::Type::NoGPUException, "Couldn't create OpenGL context!");
		
		::SetPixelFormat(_internals->hDC, _internals->pixelFormat, &descriptor);
		
		
		//if(!wgl::CreateContextAttribsARB)
		{
			HGLRC tempContext = wglCreateContext(_internals->hDC);
			if(!wglMakeCurrent(_internals->hDC, tempContext))
				throw Exception(Exception::Type::NoGPUException, "Couldn't create OpenGL context!");
			
			wgl::GetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
			if(!wgl::GetExtensionsStringARB)
				throw Exception(Exception::Type::NoGPUException, "Couldn't create OpenGL context!");
			
			std::string extensions = std::string(static_cast<const char *>(wgl::GetExtensionsStringARB(_internals->hDC)));
			
			auto createContext = extensions.find("WGL_ARB_create_context");
			auto coreProfile   = extensions.find("WGL_ARB_create_context_profile");
			
			if(createContext == std::string::npos || coreProfile == std::string::npos)
				throw Exception(Exception::Type::NoGPUException, "Couldn't create OpenGL context!");
			
			wgl::CreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
			wgl::ChoosePixelFormatARB    = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
			wgl::SwapIntervalEXT         = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
			
			wglMakeCurrent(_internals->hDC, nullptr);
			wglDeleteContext(tempContext);
		}
		
		CreateOpenGLContext(version, _internals->hDC, nullptr, &_internals->context);
#endif
	}
	
#if RN_PLATFORM_WINDOWS
	Context::Context(Context *shared, HWND window)
#else
	Context::Context(Context *shared)
#endif
	{
		RN_ASSERT(shared, "Creating a shared context but no context to share with provided!");
		Initialize(shared->_version);
		
		_shared = shared->Retain();

#if RN_PLATFORM_MAC_OS
		_internals->pixelFormat = [_shared->_internals->pixelFormat retain];
		
		_internals->context = [[NSOpenGLContext alloc] initWithFormat:_internals->pixelFormat shareContext:_shared->_internals->context];
		
		if(!_internals->context)
			throw Exception(Exception::Type::NoGPUException, "Couldn't create OpenGL context!");
		
		_internals->cglContext = static_cast<CGLContextObj>([_internals->context CGLContextObj]);
		
		if(_shared->_active && shared->_thread->OnThread())
		{
			_shared->Deactivate();
			_shared->Activate();
		}
#endif

#if RN_PLATFORM_IOS

		EAGLSharegroup *sharegroup = _shared ? [(EAGLContext *)_shared->_oglContext sharegroup] : nil;

		_oglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2 sharegroup:sharegroup];
		if(!_oglContext)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);
#endif

#if RN_PLATFORM_WINDOWS
		
		_internals->hWnd = window ? window : CreateOffscreenWindow();
		_internals->hDC  = ::GetDC(_internals->hWnd);
		_internals->ownsWindow = (window != nullptr);
		
		_internals->pixelFormat = _shared->_internals->pixelFormat;
		
		PIXELFORMATDESCRIPTOR descriptor;

		::DescribePixelFormat(_internals->hDC, _internals->pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &descriptor);
		::SetPixelFormat(_internals->hDC, _internals->pixelFormat, &descriptor);

		CreateOpenGLContext(_version, _internals->hDC, _shared->_internals->context, &_internals->context);

		if(_shared->_active && shared->_thread->OnThread())
		{
			_shared->Deactivate();
			_shared->Activate();
		}

		wglShareLists(_internals->context, _shared->_internals->context);
#endif

#if RN_PLATFORM_LINUX

		// TODO: identify which other attributes are needed here
		Colormap             cmap;
		XSetWindowAttributes swa;
		static int attributes[]  = {GLX_RGBA, 
					GLX_RED_SIZE, 8,
					GLX_GREEN_SIZE, 8,
					GLX_BLUE_SIZE, 8,
					GLX_DEPTH_SIZE, 16, 
					GLX_DOUBLEBUFFER, None};
		int dummy;

		if (_dpy == 0)
		{
			/*** (1) open a connection to the X server ***/
			_dpy = XOpenDisplay(NULL);
			if (_dpy == NULL)
				throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed, "could not open display");
		}

		/*** (2) make sure OpenGL's GLX extension supported ***/
		if(!glXQueryExtension(_dpy, &dummy, &dummy))
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed, "X server has no OpenGL GLX extension");

		/*** (3) find an appropriate visual ***/

		// find an OpenGL-capable RGB visual with depth and double buffer 
		_vi = glXChooseVisual(_dpy, DefaultScreen(_dpy), attributes);
		if (_vi == NULL)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed, "no RGB visual with depth buffer and double buffer");

		if (_vi->c_class != TrueColor)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed, "TrueColor visual required for this program");

		/*** (4) create an OpenGL rendering context  ***/

		// create an OpenGL rendering context
		_context = glXCreateContext(_dpy, _vi, _shared ? _shared->_context : 0,
							/* direct rendering if possible */ GL_TRUE);
		if (_context == NULL)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed, "could not create rendering context");

		// Create fake Invisible XWindow to enable openGl context without window 
		// create an X colormap since probably not using default visual 
		cmap = XCreateColormap(_dpy, RootWindow(_dpy, _vi->screen), _vi->visual, AllocNone);
		swa.colormap = cmap;
		swa.border_pixel = 0;
		swa.event_mask = KeyPressMask  | KeyReleaseMask | ExposureMask
					 | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask
					  | PointerMotionMask | EnterWindowMask | LeaveWindowMask;

		_win = XCreateWindow(_dpy, RootWindow(_dpy, _vi->screen), 0, 0,
						  1024, 768, 0, _vi->depth, InputOutput, _vi->visual,
						  CWBorderPixel | CWColormap | CWEventMask, &swa);
						  
		// register interest in the delete window message
	   Atom wmDeleteMessage = XInternAtom(_dpy, "WM_DELETE_WINDOW", False);
	   XSetWMProtocols(_dpy, _win, &wmDeleteMessage, 1);
						  
						  
		if(_shared)
		{
			if(_shared->_active && shared->_thread->OnThread())
			{
				_shared->Deactivate();
				_shared->Activate();
			}
		}
#endif
	}

	Context::~Context()
	{
		DeactivateContext();

		if(_shared)
			_shared->Release();

#if RN_PLATFORM_MAC_OS
		[_internals->context release];
		[_internals->pixelFormat release];
#endif

#if RN_PLATFORM_WINDOWS
		if(_internals->ownsWindow)
			::DestroyWindow(_internals->hWnd);
#endif

#if RN_PLATFORM_IOS
		[(EAGLContext *)_oglContext release];
#endif

#if RN_PLATFORM_LINUX
		glXDestroyContext(_dpy, _context);
		XDestroyWindow(_dpy, _win);
#endif
	}

	void Context::Initialize(gl::Version version)
	{
		_active  = false;
		_thread  = nullptr;
		_shared  = nullptr;
		_version = version;
		_firstActivation = true;
		
#if RN_PLATFORM_MAC_OS
		_internals->context = nil;
		_internals->pixelFormat = nil;
#endif
	}

	void Context::MakeActiveContext()
	{
		Thread *thread = Thread::GetCurrentThread();
		RN_ASSERT(thread, "");

		thread->_mutex.Lock();

		if(thread->_context == this)
		{
			thread->_mutex.Unlock();
			return;
		}

		if(thread->_context)
		{
			Context *other = thread->_context;
			other->_active = false;
			other->_thread = 0;

			other->Deactivate();
		}

		Activate();

		_active = true;
		_thread = thread;

		thread->_context = this;
		thread->_mutex.Unlock();
	}

	void Context::DeactivateContext()
	{
		Thread *thread = Thread::GetCurrentThread();
		RN_ASSERT(thread, "");

		thread->_mutex.Lock();

		_active = false;
		_thread = 0;

		Deactivate();

		thread->_context = 0;
		thread->_mutex.Unlock();
	}
	
	void Context::ForceDeactivate()
	{
		_active = false;
		_thread = 0;
		
		Deactivate();
	}

	Context *Context::GetActiveContext()
	{
		Thread *thread = Thread::GetCurrentThread();
		RN_ASSERT(thread, "");

		return thread->_context;
	}

	void Context::Activate()
	{
#if RN_PLATFORM_MAC_OS
		[_internals->context makeCurrentContext];
#endif

#if RN_PLATFORM_IOS
		[EAGLContext setCurrentContext:(EAGLContext *)_oglContext];
#endif

#if RN_PLATFORM_WINDOWS
		wglMakeCurrent(_internals->hDC, _internals->context);
#endif

#if RN_PLATFORM_LINUX
		glXMakeCurrent(_dpy, _win, _context);
#endif

		if(_firstActivation)
		{
			BindOpenGLCore();
			BindOpenGLFunctions(_version);

			_firstActivation = false;
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
		wglMakeCurrent(_internals->hDC, nullptr);
#endif

#if RN_PLATFORM_LINUX
		glXMakeCurrent(_dpy, None, 0);
#endif
	}
}
