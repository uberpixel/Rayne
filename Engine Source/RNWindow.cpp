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

@interface RNOpenGLView : NSOpenGLView
{
@public
	BOOL _sizeChanged;
	NSSize _size;
	
	RN::Context *_context;
	RN::RendererBackend *_renderer;
}
@end

@implementation RNOpenGLView

- (void)drawFrame
{
	_context->MakeActiveContext();
	
	if(_sizeChanged)
	{
		_renderer->SetDefaultFrame(_size.width, _size.height);
		_sizeChanged = NO;
	}
	
	_renderer->DrawFrame();
	
	_context->Flush();
	_context->DeactivateContext();
}

- (void)setFrame:(NSRect)frameRect
{
	[super setFrame:frameRect];
	
	_size = frameRect.size;
	_sizeChanged = YES;
}

@end

static CVReturn RNDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *now, const CVTimeStamp *outputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *context)
{
	@autoreleasepool
	{
		[(RNOpenGLView *)context drawFrame];
	}
	
	return kCVReturnSuccess;
}

@implementation RNNativeWindow

- (void)setRNContext:(RN::Context *)rnContext Renderer:(RN::RendererBackend *)renderer OpenGLContext:(NSOpenGLContext *)context andPixelFormat:(NSOpenGLPixelFormat *)pixelFormat
{
	CVDisplayLinkStop(_displayLink);
	CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(_displayLink, (CGLContextObj)[context CGLContextObj], (CGLPixelFormatObj)[pixelFormat CGLPixelFormatObj]);
	
	[_openGLView setOpenGLContext:context];
	[_openGLView setPixelFormat:pixelFormat];
	
	((RNOpenGLView *)_openGLView)->_context  = rnContext;
	((RNOpenGLView *)_openGLView)->_renderer = renderer;
	
	CVDisplayLinkStart(_displayLink);
}

- (id)initWithFrame:(NSRect)frame
{
	if((self = [super initWithContentRect:frame styleMask:NSTitledWindowMask | NSClosableWindowMask backing:NSBackingStoreBuffered defer:NO]))
	{
		NSRect rect = [self contentRectForFrameRect:frame];
		_openGLView = [[RNOpenGLView alloc] initWithFrame:rect];
		
		[self setContentView:_openGLView];
		
		CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
		CVDisplayLinkSetOutputCallback(_displayLink, &RNDisplayLinkCallback, (void *)_openGLView);
	}
	
	return self;
}

- (void)dealloc
{
	CVDisplayLinkRelease(_displayLink);
	
	[_openGLView release];
	[super dealloc];
}

@end

#endif

#if RN_PLATFORM_IOS

@interface RNOpenGLView : UIView
{
	GLuint _frameBuffer;
	GLuint _colorBuffer;
	
	RN::Context *_context;
	RN::RendererBackend *_renderer;
	
	NSThread *_rendererThread;
	CADisplayLink *_displayLink;
	
	GLint _backingWidth;
	GLint _backingHeight;
	
	BOOL _resizeLayer;
}

@property (nonatomic, readonly) GLint _backingWidth;
@property (nonatomic, readonly) GLint _backingHeight;

@end

@implementation RNOpenGLView
@synthesize _backingWidth, _backingHeight;

+ (Class)layerClass
{
	return [CAEAGLLayer class];
}

// Rendering

- (void)drawBackgroundFrame
{
	if(_resizeLayer)
		[self resizeFromLayer:(CAEAGLLayer *)[self layer]];
	
	_renderer->DrawFrame();
	
	glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, _colorBuffer);
	
	[[EAGLContext currentContext] presentRenderbuffer:GL_RENDERBUFFER];
}

- (void)renderTest
{
	_context->MakeActiveContext();
	
	[self createDrawBuffer];
	[self resizeFromLayer:(CAEAGLLayer *)[self layer]];
	
	_displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawBackgroundFrame)];
	[_displayLink setFrameInterval:1];
	[_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
	
	while(![_rendererThread isCancelled])
	{
		NSDate *date = [NSDate dateWithTimeIntervalSinceNow:0.1];
		[[NSRunLoop currentRunLoop] runUntilDate:date];
	}
	
	[_displayLink removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
}


// Buffer Management

- (void)createDrawBuffer
{
	glGenFramebuffers(1, &_frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
	
	glGenRenderbuffers(1, &_colorBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, _colorBuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorBuffer);
	
	_renderer->SetDefaultFBO(_frameBuffer);
}

- (void)resizeFromLayer:(CAEAGLLayer *)layer
{
	glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, _colorBuffer);
	[[EAGLContext currentContext] renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];
	
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH,  &_backingWidth);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &_backingHeight);
	
	_renderer->SetDefaultFrame(_backingWidth, _backingHeight);
	_resizeLayer = NO;
}

- (void)layoutSubviews
{
	[super layoutSubviews];
	
	_resizeLayer = YES;
}

- (id)initWithContext:(RN::Context *)context renderer:(RN::RendererBackend *)renderer andFrame:(CGRect)frame
{
	if((self = [super initWithFrame:frame]))
	{
		_renderer = renderer;
		_context = new RN::Context(context);
		_context->SetName("Renderering Context");
		
		CAEAGLLayer *layer = (CAEAGLLayer *)[self layer];
		NSDictionary *properties = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
		
		[layer setDrawableProperties:properties];
		[layer setOpaque:YES];
		
		_rendererThread = [[NSThread alloc] initWithTarget:self selector:@selector(renderTest) object:nil];
		[_rendererThread start];
	}
	
	return self;
}

- (void)dealloc
{
	// Wait for the rendering thread to terminate
	
	[_rendererThread cancel];
	
	while([_rendererThread isExecuting])
		[NSThread sleepForTimeInterval:0.1f];
	
	[_rendererThread release];
	_context->Release();
	
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


- (id)initWithContext:(RN::Context *)context renderer:(RN::RendererBackend *)renderer andFrame:(CGRect)frame
{
	if((self = [super init]))
	{
		openGLView = [[RNOpenGLView alloc] initWithContext:context renderer:renderer andFrame:frame];
		[openGLView autorelease];
		
		[self setView:openGLView];
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
		
		_nativeWindow->_controller = this;

		_context = 0;
		_kernel = kernel;
		
		SetTitle(title);
		Show();
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
		
		[_nativeWindow setRNContext:_context Renderer:_kernel->RendererBackend() OpenGLContext:_context->_oglContext andPixelFormat:_context->_oglPixelFormat];
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
#endif
	
	
#if RN_PLATFORM_IOS
	Window::Window(const std::string& title, Kernel *kernel)
	{
		_nativeWindow = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] applicationFrame]];
		[_nativeWindow setBackgroundColor:[UIColor whiteColor]];
		
		_rootViewController = 0;

		_context = 0;
		_kernel = kernel;
		
		SetTitle(title);
		Show();
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
		_context->Release();
		_context = context;
		_context->Retain();
		
		[_rootViewController release];
		_rootViewController = [[RNOpenGLViewController alloc] initWithContext:_context renderer:_kernel->RendererBackend() andFrame:[_nativeWindow bounds]];
		_renderingView = [_rootViewController view];
		
		[_nativeWindow setRootViewController:_rootViewController];
	}
	
	void Window::SetTitle(const std::string& title)
	{
	}
	
	Rect Window::Frame() const
	{
		CGRect frame = [_renderingView bounds];
		return Rect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
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
		SetTitle(title);
	}

	Window::~Window()
	{
	}

	void Window::Show()
	{
	}

	void Window::Hide()
	{
	}

	void Window::SetContext(Context *context)
	{
	}

	void Window::SetTitle(const std::string& title)
	{
	}

	Rect Window::Frame() const
	{
		return Rect(Vector2(0.0f, 0.0f), Vector2(0.0f, 0.0f));
	}
}

LRESULT CALLBACK RNWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void RNRegisterWindow()
{
	static bool registered = false;
	if(!registered)
	{
		WNDCLASSEX windowClass;
		memset(&windowClass, 0, sizeof(WNDCLASSEX));

		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		windowClass.lpfnWndProc = RNWndProc;
		windowClass.hInstance = (HINSTANCE)GetModuleHandle(0);
		windowClass.hIcon = LoadIcon(0, IDI_APPLICATION);
		windowClass.hCursor = LoadCursor(0, IDC_ARROW);
		windowClass.lpszClassName = (LPCWSTR)"RNWindowClass";
		windowClass.hIconSm = LoadIcon(0, IDI_WINLOGO);

		RegisterClassEx(&windowClass);
		registered = true;
	}
}

#endif
