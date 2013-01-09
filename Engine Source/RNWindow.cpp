//
//  RNWindow.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
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
	RN::Context *_context;
	RN::RendererBackend *_renderer;
}
@end

@implementation RNOpenGLView

- (void)drawFrame
{
	_context->MakeActiveContext();
	
	try
	{
		_renderer->DrawFrame();
	}
	catch(RN::ErrorException e)
	{
		uint32 group = e.Group();
		uint32 subgroup = e.Subgroup();
		uint32 code = e.Code();
		
		if(code != RN::kGraphicsFramebufferUndefined || subgroup != RN::kGraphicsGroupOpenGL || group != RN::kErrorGroupGraphics)
			throw e; // Rethrow
	}
	
	_context->Flush();
	_context->DeactivateContext();
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
	RN::Camera *_camera;
	RN::RendererBackend *_renderer;
	
	NSThread *_rendererThread;
	CADisplayLink *_displayLink;
	
	GLint _backingWidth;
	GLint _backingHeight;
	
	BOOL _resizeLayer;
}

@end

@implementation RNOpenGLView

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
	
	CGRect frame = [self frame];
	
	_camera = new RN::Camera(_frameBuffer, RN::Vector2(frame.size.width, frame.size.height));
	_renderer->SetDefaultCamera(_camera);
}

- (void)resizeFromLayer:(CAEAGLLayer *)layer
{
	_camera->Bind();
	
	glBindRenderbuffer(GL_RENDERBUFFER, _colorBuffer);
	[[EAGLContext currentContext] renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];
	
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH,  &_backingWidth);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &_backingHeight);
	
	_camera->SetFrame(RN::Rect(0.0f, 0.0f, _backingWidth, _backingHeight));
	_camera->UpdateProjection();
	_camera->Unbind();
	
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
	
	_renderer->SetDefaultCamera(0);
	_camera->Release();
	
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
	Window::Window(const std::string& title)
	{
#if RN_PLATFORM_MAC_OS 
		_nativeWindow = [[RNNativeWindow alloc] initWithFrame:NSMakeRect(0, 0, 1024, 768)];
		[_nativeWindow center];
		
		_nativeWindow->_controller = this;
#endif
		
#if RN_PLATFORM_IOS
		_nativeWindow = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] applicationFrame]];
		[_nativeWindow setBackgroundColor:[UIColor whiteColor]];
		
		_rootViewController = 0;
#endif
		_context = 0;
		
		SetTitle(title);
		Show();
	}
	
	Window::~Window()
	{
		_context->Release();
		[_nativeWindow release];
		
#if RN_PLATFORM_IOS
		[_rootViewController release];
#endif
	}
	
	
	void Window::Show()
	{
#if RN_PLATFORM_MAC_OS 
		[_nativeWindow makeKeyAndOrderFront:nil];
#endif
#if RN_PLATFORM_IOS
		[_nativeWindow makeKeyAndVisible];
#endif
	}
	
	void Window::Hide()
	{
#if RN_PLATFORM_MAC_OS 
		[_nativeWindow close];
#endif
#if RN_PLATFORM_IOS
		[_nativeWindow resignFirstResponder];
		[_nativeWindow setHidden:YES];
#endif
	}
	
	void Window::SetContext(Context *context, RendererBackend *renderer)
	{
#if RN_PLATFORM_MAC_OS 
		_context->Release();
		_context = new Context(context);
		
		[_nativeWindow setRNContext:_context Renderer:renderer OpenGLContext:_context->_oglContext andPixelFormat:_context->_oglPixelFormat];
#endif
		
#if RN_PLATFORM_IOS
		_context->Release();
		_context = context;
		_context->Retain();
		
		[_rootViewController release];
		_rootViewController = [[RNOpenGLViewController alloc] initWithContext:_context renderer:_kernel->RendererBackend() andFrame:[_nativeWindow bounds]];
		
		[_nativeWindow setRootViewController:_rootViewController];
#endif
	}
	
	void Window::SetTitle(const std::string& title)
	{
#if RN_PLATFORM_MAC_OS 
		[_nativeWindow setTitle:[NSString stringWithUTF8String:title.c_str()]];
#endif
	}
}

#endif
