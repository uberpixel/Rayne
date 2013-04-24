//
//  RNContext.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNContext.h"
#include "RNBaseInternal.h"
#include "RNMutex.h"
#include "RNSettings.h"

#if RN_PLATFORM_WINDOWS
extern void RNRegisterWindow();
#endif

namespace RN
{
	RNDeclareMeta(Context)
	
#if RN_PLATFORM_LINUX
	Display *Context::_dpy = 0;
#endif 
	
	Context::Context(Context *shared)
	{
		_active = false;
		_thread = 0;
		_shared = shared ? shared->Retain() : 0;
		_firstActivation = true;

#if RN_PLATFORM_MAC_OS
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

		_oglPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:formatAttributes];
		if(!_oglPixelFormat)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsNoHardware);

		_oglContext = [[NSOpenGLContext alloc] initWithFormat:(NSOpenGLPixelFormat *)_oglPixelFormat shareContext:_shared ? (NSOpenGLContext *)_shared->_oglContext : nil];
		if(!_oglContext)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsContextFailed);

		_cglContext = (CGLContextObj)[(NSOpenGLContext *)_oglContext CGLContextObj];
		CGLEnable((CGLContextObj)_cglContext, kCGLCEMPEngine);
		
		if(_shared)
		{
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

#elif RN_PLATFORM_LINUX

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

#if RN_PLATFORM_LINUX
		glXDestroyContext(_dpy, _context);
		XDestroyWindow(_dpy, _win);
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
	
	
	void Context::SetDepthClear(GLfloat depth)
	{
		if(depth != _depthClear)
		{
#if RN_TARGET_OPENGL_ES
			glClearDepthf(depth);
#endif
			
#if RN_TARGET_OPENGL
			glClearDepth(depth);
#endif
			_depthClear = depth;
		}
	}
	void Context::SetStencilClear(GLint stencil)
	{
		if(stencil != _stencilClear)
		{
			glClearStencil(stencil);
			_stencilClear = stencil;
		}
	}
	void Context::SetClearColor(const Color& color)
	{
		if(_clearColor != color)
		{
			glClearColor(color.r, color.g, color.b, color.a);
			_clearColor = color;
		}
	}
	

	Context *Context::ActiveContext()
	{
		Thread *thread = Thread::CurrentThread();
		RN_ASSERT0(thread);

		return thread->_context;
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

#if RN_PLATFORM_LINUX
		glXMakeCurrent(_dpy, _win, _context);
#endif

		if(_firstActivation)
		{
			_firstActivation = false;
			
			glGetFloatv(GL_DEPTH_CLEAR_VALUE, &_depthClear);
			glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &_stencilClear);
			glGetFloatv(GL_COLOR_CLEAR_VALUE, &_clearColor.r);
			
#if GL_FRAMEBUFFER_SRGB
			if(Settings::SharedInstance()->GammaCorrection())
			{
				glEnable(GL_FRAMEBUFFER_SRGB);
			}
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

#if RN_PLATFORM_LINUX
		glXMakeCurrent(_dpy, None, 0);
#endif
	}
}
