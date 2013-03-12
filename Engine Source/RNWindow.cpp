//
//  RNWindow.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWindow.h"
#include "RNBaseInternal.h"

#include "RNFile.h"
#include "RNTexture.h"
#include "RNKernel.h"
#include "RNInput.h"

#if RN_PLATFORM_MAC_OS

@interface RNNativeWindow : NSWindow <NSWindowDelegate>
{
	NSOpenGLView *_openGLView;
	BOOL _needsResize;
}

@property (nonatomic, assign) BOOL needsResize;
@end

@implementation RNNativeWindow
@synthesize needsResize = _needsResize;

- (BOOL)windowShouldClose:(id)sender
{
	RN::Kernel::SharedInstance()->Exit();
	return NO;
}

- (void)keyDown:(NSEvent *)theEvent
{
}

- (void)keyUp:(NSEvent *)theEvent
{
}

- (void)mouseDown:(NSEvent *)theEvent
{
}

- (void)mouseMoved:(NSEvent *)theEvent
{
}

- (void)mouseUp:(NSEvent *)theEvent
{
}

- (void)mouseDragged:(NSEvent *)theEvent
{
}


- (BOOL)canBecomeKeyWindow
{
	return YES;
}

- (BOOL)canBecomeMainWindow
{
	return YES;
}


- (void)setOpenGLContext:(NSOpenGLContext *)context andPixelFormat:(NSOpenGLPixelFormat *)pixelFormat
{
	[_openGLView setOpenGLContext:context];
	[_openGLView setPixelFormat:pixelFormat];
}

- (id)initWithFrame:(NSRect)frame
{
	if((self = [super initWithContentRect:frame styleMask:NSTitledWindowMask | NSClosableWindowMask backing:NSBackingStoreBuffered defer:NO]))
	{
		NSRect rect = [self contentRectForFrameRect:frame];
		_openGLView = [[NSOpenGLView alloc] initWithFrame:rect];
		[_openGLView  setWantsBestResolutionOpenGLSurface:YES];

		[self setContentView:_openGLView];
		[self setDelegate:self];

		_needsResize = YES;
	}

	return self;
}

- (void)dealloc
{	
	[_openGLView release];
	[super dealloc];
}

@end

#endif

#if RN_PLATFORM_IOS

@interface RNOpenGLView : UIView
{
	CAEAGLLayer *_renderLayer;
	RN::Window *_controller;

	GLuint _framebuffer;
	GLuint _colorbuffer;

	GLint _backingWidth;
	GLint _backingHeight;

	BOOL _needsLayerResize;
}

@property (nonatomic, readonly) BOOL needsLayerResize;
@property (nonatomic, readonly) GLuint framebuffer;
@property (nonatomic, readonly) GLint backingWidth;
@property (nonatomic, readonly) GLint backingHeight;

@end

@implementation RNOpenGLView
@synthesize needsLayerResize = _needsLayerResize;
@synthesize framebuffer = _framebuffer;
@synthesize backingWidth = _backingWidth, backingHeight = _backingHeight;

+ (Class)layerClass
{
	return [CAEAGLLayer class];
}

// Input

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	for(UITouch *touch in touches)
	{
		RN::Touch temp;
		CGPoint location = [touch locationInView:[touch view]];

		temp.phase = RN::Touch::TouchPhaseBegan;
		temp.location = RN::Vector2(location.x, location.y);
		temp.previousLocation = RN::Vector2();

		RN::Input::SharedInstance()->HandleTouchEvent(temp);
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	for(UITouch *touch in touches)
	{
		RN::Touch temp;
		CGPoint location = [touch locationInView:[touch view]];
		CGPoint prevLocation = [touch previousLocationInView:[touch view]];

		temp.phase = RN::Touch::TouchPhaseMoved;
		temp.location = RN::Vector2(location.x, location.y);
		temp.previousLocation = RN::Vector2(prevLocation.x, prevLocation.y);

		RN::Input::SharedInstance()->HandleTouchEvent(temp);
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	for(UITouch *touch in touches)
	{
		RN::Touch temp;
		CGPoint location = [touch locationInView:[touch view]];
		CGPoint prevLocation = [touch previousLocationInView:[touch view]];

		temp.phase = RN::Touch::TouchPhaseEnded;
		temp.location = RN::Vector2(location.x, location.y);
		temp.previousLocation = RN::Vector2(prevLocation.x, prevLocation.y);

		RN::Input::SharedInstance()->HandleTouchEvent(temp);
	}
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	for(UITouch *touch in touches)
	{
		RN::Touch temp;
		CGPoint location = [touch locationInView:[touch view]];
		CGPoint prevLocation = [touch previousLocationInView:[touch view]];

		temp.phase = RN::Touch::TouchPhaseCancelled;
		temp.location = RN::Vector2(location.x, location.y);
		temp.previousLocation = RN::Vector2(prevLocation.x, prevLocation.y);

		RN::Input::SharedInstance()->HandleTouchEvent(temp);
	}
}

// Rendering

- (void)flushFrame
{
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, _colorbuffer);

	[[EAGLContext currentContext] presentRenderbuffer:GL_RENDERBUFFER];
}

// Buffer Management

- (void)createDrawBuffer
{
	glGenFramebuffers(1, &_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

	glGenRenderbuffers(1, &_colorbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, _colorbuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorbuffer);

	_needsLayerResize = YES;
}

- (void)resizeFromLayer
{
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, _colorbuffer);

	[[EAGLContext currentContext] renderbufferStorage:GL_RENDERBUFFER fromDrawable:_renderLayer];

	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH,  &_backingWidth);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &_backingHeight);

	float scaleFactor = RN::Kernel::SharedInstance()->ScaleFactor();

	_backingWidth  /= scaleFactor;
	_backingHeight /= scaleFactor;

	_needsLayerResize = NO;
}

- (void)layoutSubviews
{
	[super layoutSubviews];
	_needsLayerResize = YES;
}

- (id)initWithController:(RN::Window *)controller andFrame:(CGRect)frame
{
	if((self = [super initWithFrame:frame]))
	{
		[self setMultipleTouchEnabled:YES];

		_controller = controller;

		NSDictionary *properties = @{kEAGLDrawablePropertyRetainedBacking : @NO,  kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8};

		_renderLayer = (CAEAGLLayer *)[self layer];

		[_renderLayer setContentsScale:RN::Kernel::SharedInstance()->ScaleFactor()];
		[_renderLayer setDrawableProperties:properties];
		[_renderLayer setOpaque:YES];
	}

	return self;
}

- (void)dealloc
{
	glDeleteRenderbuffers(1, &_colorbuffer);
	glDeleteFramebuffers(1, &_framebuffer);

	[super dealloc];
}

@end

@interface RNOpenGLViewController : UIViewController
{
	RNOpenGLView *openGLView;
}

@property (nonatomic, readonly) RNOpenGLView *openGLView;

@end

@implementation RNOpenGLViewController
@synthesize openGLView;

- (NSUInteger)supportedInterfaceOrientations
{
	return UIInterfaceOrientationMaskLandscape;
}

- (BOOL)shouldAutorotate
{
	return YES;
}


- (id)initWithController:(RN::Window *)controller andFrame:(CGRect)frame
{
	if((self = [super init]))
	{
		openGLView = [[RNOpenGLView alloc] initWithController:controller andFrame:frame];
		[self setView:openGLView];
		[openGLView release];
	}

	return self;
}

@end

#endif


#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS

namespace RN
{
	Window::Window(const std::string& title, Kernel *kernel)
	{
#if RN_PLATFORM_MAC_OS
		_nativeWindow = [[RNNativeWindow alloc] initWithFrame:NSMakeRect(0, 0, 1024, 768)];
		[(RNNativeWindow *)_nativeWindow center];
#endif

#if RN_PLATFORM_IOS
		_nativeWindow = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] applicationFrame]];
		[(UIWindow *)_nativeWindow setBackgroundColor:[UIColor whiteColor]];

		_rootViewController = 0;
#endif

		_context = 0;
		_kernel = kernel;
		_renderer = _kernel->Renderer();

		SetTitle(title);

		_thread = new Thread(std::bind(&Window::RenderLoop, this));
		_renderer->SetThread(_thread);
	}

	Window::~Window()
	{
		_context->Release();
		_thread->Release();

#if RN_PLATFORM_MAC_OS
		[(RNNativeWindow *)_nativeWindow release];
#endif

#if RN_PLATFORM_IOS
		[(UIWindow *)_nativeWindow release];
		[(UIViewController *)_rootViewController release];
#endif
	}


	void Window::Show()
	{
#if RN_PLATFORM_MAC_OS
		[(RNNativeWindow *)_nativeWindow makeKeyAndOrderFront:nil];
#endif

#if RN_PLATFORM_IOS
		[(UIWindow *)_nativeWindow makeKeyAndVisible];
#endif
	}

	void Window::Hide()
	{
#if RN_PLATFORM_MAC_OS
		[(RNNativeWindow *)_nativeWindow close];
#endif

#if RN_PLATFORM_IOS
		[(UIWindow *)_nativeWindow resignFirstResponder];
		[(UIWindow *)_nativeWindow setHidden:YES];
#endif
	}

	void Window::SetContext(Context *context)
	{
#if RN_PLATFORM_IOS
		[(UIViewController *)_rootViewController release];

		_rootViewController = [[RNOpenGLViewController alloc] initWithController:this andFrame:[(UIWindow *)_nativeWindow bounds]];
		_renderingView      = (RNOpenGLView *)[(UIViewController *)_rootViewController view];

		[(UIWindow *)_nativeWindow setRootViewController:(UIViewController *)_rootViewController];
#endif

		_context->Release();
		_context = new Context(context);

#if RN_PLATFORM_MAC_OS
		[(RNNativeWindow *)_nativeWindow setOpenGLContext:(NSOpenGLContext *)_context->_oglContext andPixelFormat:(NSOpenGLPixelFormat *)_context->_oglPixelFormat];
#endif
	}

	void Window::SetTitle(const std::string& title)
	{
#if RN_PLATFORM_MAC_OS
		[(RNNativeWindow *)_nativeWindow setTitle:[NSString stringWithUTF8String:title.c_str()]];
#endif
	}

	Rect Window::Frame() const
	{
#if RN_PLATFORM_MAC_OS
		NSRect frame = [[(RNNativeWindow *)_nativeWindow contentView] frame];
		return Rect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
#endif

#if RN_PLATFORM_IOS
		CGRect frame = [(RNOpenGLView *)_renderingView bounds];
		return Rect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
#endif
	}

	void Window::RenderLoop()
	{
		while(!_context)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(5));
			continue;
		}

		_context->MakeActiveContext();

#if RN_PLATFORM_IOS
		[(RNOpenGLView *)_renderingView createDrawBuffer];
#endif

		while(!_thread->IsCancelled())
		{
#if RN_PLATFORM_MAC_OS
			if(((RNNativeWindow *)_nativeWindow).needsResize)
			{
				NSRect frame = [[(RNNativeWindow *)_nativeWindow contentView] frame];

				_renderer->SetDefaultFrame(frame.size.width, frame.size.height);
				((RNNativeWindow *)_nativeWindow).needsResize = false;
			}

			if(!_renderer->WaitForWork())
				break;
			
			CGLFlushDrawable((CGLContextObj)[(NSOpenGLContext *)_context->_oglContext CGLContextObj]);
#endif

#if RN_PLATFORM_IOS
			if(((RNOpenGLView *)_renderingView).needsLayerResize)
			{
				[(RNOpenGLView *)_renderingView resizeFromLayer];

				_renderer->SetDefaultFrame(((RNOpenGLView *)_renderingView).backingWidth, ((RNOpenGLView *)_renderingView).backingHeight);
				_renderer->SetDefaultFBO(((RNOpenGLView *)_renderingView).framebuffer);
			}

			_renderer->WaitForWork();
			[(RNOpenGLView *)_renderingView flushFrame];
#endif
		}

		_context->DeactivateContext();
	}
}
#endif

#if RN_PLATFORM_WINDOWS

void RNRegisterWindow();

namespace RN
{
	Window::Window(const std::string& title, Kernel *kernel)
	{
		RNRegisterWindow();

		_stopRendering = false;
		_threadStopped = false;

		_hWnd = 0;
		_title = title;
		_kernel = kernel;
		_context = 0;
	}

	Window::~Window()
	{
		_stopRendering = true;

		while(!_threadStopped)
		{}

		_context->Release();
	}

	void Window::Show()
	{
		ShowWindow(_hWnd, SW_SHOW);
		UpdateWindow(_hWnd);
	}

	void Window::Hide()
	{
		ShowWindow(_hWnd, SW_HIDE);
	}

	void Window::RenderLoop()
	{
		_context->MakeActiveContext();

		while(!_stopRendering)
		{
			_renderer->DrawFrame();
			SwapBuffers(_hDC);
		}

		Thread::CurrentThread()->Exit();
		_threadStopped = true;
	}

	void Window::SetContext(Context *context)
	{
		_context->Release();
		_context = new Context(context);

		_hWnd = _context->_hWnd;
		_hDC  = _context->_hDC;

		SetWindowLongPtr(_hWnd, GWL_USERDATA, (LONG_PTR)this);
		SetTitle(_title);

		Rect frame = Frame();

		_renderer = _kernel->RendererBackend();
		_renderer->SetDefaultFrame(frame.width, frame.height);

		std::thread temp = std::thread(&Window::RenderLoop, this);
		temp.detach();
	}

	void Window::SetTitle(const std::string& title)
	{
		if(_hWnd)
			SetWindowTextA(_hWnd, (LPCSTR)title.c_str());

		_title = title;
	}

	Rect Window::Frame() const
	{
		RECT rect;
		GetClientRect(_hWnd, &rect);

		return Rect(Vector2(rect.left, rect.top), Vector2(rect.right - rect.left, rect.bottom - rect.top));
	}
}

LRESULT CALLBACK RNWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RN::Window *window = (RN::Window *)GetWindowLongPtr(hWnd, GWL_USERDATA);

	switch(message)
	{
		case WM_DESTROY:
			if(window)
			{
				RN::Kernel::SharedInstance()->Exit();
			}
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

void RNRegisterWindow()
{
	static bool registered = false;
	if(!registered)
	{
		WNDCLASSEXA windowClass;
		memset(&windowClass, 0, sizeof(WNDCLASSEXA));

		windowClass.cbSize = sizeof(WNDCLASSEXA);
		windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		windowClass.lpfnWndProc = RNWndProc;
		windowClass.hInstance = (HINSTANCE)GetModuleHandle(0);
		windowClass.hIcon = LoadIcon(0, IDI_APPLICATION);
		windowClass.hCursor = LoadCursor(0, IDC_ARROW);
		windowClass.lpszClassName = "RNWindowClass";
		windowClass.hIconSm = LoadIcon(0, IDI_WINLOGO);

		RegisterClassExA(&windowClass);
		registered = true;
	}
}

}
#endif // RN_PLATFORM_WINDOWS



#if RN_PLATFORM_LINUX


namespace RN
{
	Window::Window(const std::string& title, Kernel *kernel)
	{
		_context = 0;
		_win = 0;
		_title.assign(title);

		_kernel = kernel;
		_renderer = _kernel->Renderer();

		_thread = new Thread(std::bind(&Window::RenderLoop, this));
		_renderer->SetThread(_thread);
	}

	Window::~Window()
	{		
		_context->Release();
		_thread->Release();
	}

	void Window::Show()
	{
		XMapWindow(_dpy, _win);
	}

	void Window::Hide()
	{
		XUnmapWindow(_dpy, _win);
	}

	void Window::SetContext(Context *context)
	{
		_renderer = _kernel->Renderer();
		_renderer->SetDefaultFrame(1024, 768);
		
		_context->Release();
		_context = new Context(context);

		_dpy = _context->_dpy;
		_win = _context->_win;

		
		SetTitle(_title);
	}

	void Window::SetTitle(const std::string& title)
	{
		XStoreName(_dpy, _win, title.c_str());
	}

	Rect Window::Frame() const
	{
		return Rect(0, 0, 1024, 768);
	}

	void Window::RenderLoop()
	{		
		while(!_context)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(5));
			continue;
		}

		_context->MakeActiveContext();

		Atom wmDeleteMessage = XInternAtom(_dpy, "WM_DELETE_WINDOW", False);
		XEvent event;
		while(!_thread->IsCancelled())
		{
			// go through xevents
			while(XPending(_dpy))
			{
				XNextEvent(_dpy, &event);
				switch (event.type)
				{
					case ConfigureNotify:
					case Expose:
					case ClientMessage:
						if (event.xclient.data.l[0] == wmDeleteMessage)
						{
							// Exit the application
							RN::Kernel::SharedInstance()->Exit();
						}
						break;
						
					default:
						Input::SharedInstance()->HandleXInputEvents(&event);
						break;
				}
			} 
			
			if(!_renderer->WaitForWork())
				break;
		
			glXSwapBuffers(_dpy, _win);
		}

		_context->DeactivateContext();
	}

}
#endif	// RN_PLATFORM_LINUX
