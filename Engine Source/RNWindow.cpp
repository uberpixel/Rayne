//
//  RNWindow.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWindow.h"
#include "RNFile.h"
#include "RNTexture.h"
#include "RNKernel.h"

#if RN_PLATFORM_MAC_OS

@implementation RNNativeWindow

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
		
		[self setContentView:_openGLView];
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
		_controller = controller;
		
		NSDictionary *properties = @{kEAGLDrawablePropertyRetainedBacking : @NO,  kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8};
		
		_renderLayer = (CAEAGLLayer *)[self layer];
		
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
#if RN_PLATFORM_MAC_OS
	Window::Window(const std::string& title, Kernel *kernel)
	{
		_nativeWindow = [[RNNativeWindow alloc] initWithFrame:NSMakeRect(0, 0, 1024, 768)];
		[_nativeWindow center];
		
		_context = 0;
		_kernel = kernel;
		_renderer = _kernel->Renderer();
		
		SetTitle(title);
		
		std::thread thread = std::thread(&Window::RenderLoop, this);
		thread.detach();
	}
	
	Window::~Window()
	{
		_context->Release();
		[_nativeWindow release];
	}
	
	
	void Window::Show()
	{
		[_nativeWindow makeKeyAndOrderFront:nil];
	}
	
	void Window::Hide()
	{
		[_nativeWindow close];
	}
	
	void Window::SetContext(Context *context)
	{
		_context->Release();
		_context = new Context(context);
		
		[_nativeWindow setOpenGLContext:_context->_oglContext andPixelFormat:_context->_oglPixelFormat];
	}
	
	void Window::SetTitle(const std::string& title)
	{
		[_nativeWindow setTitle:[NSString stringWithUTF8String:title.c_str()]];
	}
	
	Rect Window::Frame() const
	{
		NSRect frame = [[_nativeWindow contentView] frame];
		return Rect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
	}
	
	void Window::RenderLoop()
	{
		while(!_context)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(5));
			continue;
		}
		
		while(!_renderer->ShouldStop())
		{
			_context->MakeActiveContext();
			
			_renderer->WaitForWork();
			CGLFlushDrawable((CGLContextObj)[_context->_oglContext CGLContextObj]);
			
			_context->DeactivateContext();
		}
		
		_renderer->DidStop();
	}
	
#endif
	
	
#if RN_PLATFORM_IOS
	Window::Window(const std::string& title, Kernel *kernel)
	{
		_nativeWindow = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] applicationFrame]];
		[_nativeWindow setBackgroundColor:[UIColor whiteColor]];
		
		_rootViewController = 0;

		_context = 0;
		_kernel = kernel;
		_renderer = _kernel->Renderer();
		
		SetTitle(title);
		
		std::thread thread = std::thread(&Window::RenderLoop, this);
		thread.detach();
	}
	
	Window::~Window()
	{
		_context->Release();
		[_nativeWindow release];
		[_rootViewController release];
	}
	
	
	void Window::Show()
	{
		[_nativeWindow makeKeyAndVisible];
	}
	
	void Window::Hide()
	{
		[_nativeWindow resignFirstResponder];
		[_nativeWindow setHidden:YES];
	}
	
	void Window::SetContext(Context *context)
	{		
		[_rootViewController release];
		
		_rootViewController = [[RNOpenGLViewController alloc] initWithController:this andFrame:[_nativeWindow bounds]];
		_renderingView      = (RNOpenGLView *)[_rootViewController view];
		
		[_nativeWindow setRootViewController:_rootViewController];
		
		_context->Release();
		_context = new Context(context);
	}
	
	void Window::SetTitle(const std::string& title)
	{
	}
	
	Rect Window::Frame() const
	{
		CGRect frame = [_renderingView bounds];
		return Rect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
	}
	
	void Window::RenderLoop()
	{
		while(!_context)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(5));
			continue;
		}
		
		_context->MakeActiveContext();
		[_renderingView createDrawBuffer];
		_context->DeactivateContext();
		
		while(!_renderer->ShouldStop())
		{
			_context->MakeActiveContext();
			
			if(_renderingView.needsLayerResize)
			{
				[_renderingView resizeFromLayer];
				
				_renderer->SetDefaultFrame(_renderingView.backingWidth, _renderingView.backingHeight);
				_renderer->SetDefaultFBO(_renderingView.framebuffer);
			}
			
			_renderer->WaitForWork();
			[_renderingView flushFrame];
			
			_context->DeactivateContext();
		}
		
		_renderer->DidStop();		
	}	
#endif
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

#endif
