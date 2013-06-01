//
//  RNWindow.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWindow.h"
#include "RNBaseInternal.h"

#include "RNFile.h"
#include "RNTexture.h"
#include "RNKernel.h"
#include "RNInput.h"

#if RN_PLATFORM_MAC_OS

// ---------------------
// MARK: -
// MARK: NSWindow
// ---------------------

@interface RNNativeWindow : NSWindow <NSWindowDelegate>
{
	NSOpenGLView *_openGLView;
}

@end

@implementation RNNativeWindow

- (BOOL)windowShouldClose:(id)sender
{
	RN::Kernel::SharedInstance()->Exit();
	return NO;
}

- (void)keyDown:(NSEvent *)theEvent
{
	RN::Input::SharedInstance()->HandleEvent(theEvent);
}

- (void)keyUp:(NSEvent *)theEvent
{
	RN::Input::SharedInstance()->HandleEvent(theEvent);
}

- (void)mouseDown:(NSEvent *)theEvent
{
	RN::Input::SharedInstance()->HandleEvent(theEvent);
}

- (void)mouseMoved:(NSEvent *)theEvent
{
	RN::Input::SharedInstance()->HandleEvent(theEvent);
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	RN::Input::SharedInstance()->HandleEvent(theEvent);
}

- (void)mouseUp:(NSEvent *)theEvent
{
	RN::Input::SharedInstance()->HandleEvent(theEvent);
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

- (id)initWithFrame:(NSRect)frame andStyleMask:(NSUInteger)stylemask
{
	if((self = [super initWithContentRect:frame styleMask:stylemask backing:NSBackingStoreBuffered defer:NO]))
	{
		NSRect rect = [self contentRectForFrameRect:frame];
		_openGLView = [[NSOpenGLView alloc] initWithFrame:rect];
		
		[_openGLView setWantsBestResolutionOpenGLSurface:YES];

		[self setContentView:_openGLView];
		[self setDelegate:self];
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

// ---------------------
// MARK: -
// MARK: UIWindow / UIView
// ---------------------

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

namespace RN
{
	// ---------------------
	// MARK: -
	// MARK: DisplayConfiguration
	// ---------------------
	
#if RN_PLATFORM_MAC_OS
	WindowConfiguration::WindowConfiguration(CGDisplayModeRef mode)
	{
		_mode = CGDisplayModeRetain(mode);
		
		_width  = (uint32)CGDisplayModeGetPixelWidth(_mode);
		_height = (uint32)CGDisplayModeGetPixelHeight(_mode);
	}
	
	WindowConfiguration::WindowConfiguration(const WindowConfiguration& other)
	{
		_mode = CGDisplayModeRetain(other._mode);
		
		_width  = other._width;
		_height = other._height;
	}
	
	WindowConfiguration::WindowConfiguration(uint32 width, uint32 height)
	{
		_mode = 0;
		
		_width  = width;
		_height = height;
	}
	
	WindowConfiguration::~WindowConfiguration()
	{
		if(_mode)
			CGDisplayModeRelease(_mode);
	}
#endif
	
#if RN_PLATFORM_LINUX
	WindowConfiguration::WindowConfiguration(int32 index, uint32 width, uint32 height)
	{
		_modeIndex = index;
		
		_width  = width;
		_height = height;
	}
	
	WindowConfiguration::WindowConfiguration(uint32 width, uint32 height)
	{
		_modeIndex = -1;
		
		_width  = width;
		_height = height;
	}
#endif
	
	
	// ---------------------
	// MARK: -
	// MARK: Window
	// ---------------------
	
	Window::Window() :
		_activeConfiguration(WindowConfiguration(1024, 768))
	{
		_kernel = Kernel::SharedInstance();
		_context = _kernel->Context();
		
		_mask = 0;
		_activeConfiguration = 0;
		_cursorVisible = true;
		
#if RN_PLATFORM_MAC_OS
		CGDisplayCount count;
		
		CGGetActiveDisplayList(0, 0, &count);
		CGDirectDisplayID *table = new CGDirectDisplayID[count];
		
		CGGetActiveDisplayList(count, table, &count);
		for(machine_uint i=0; i<count; i++)
		{
			CGDirectDisplayID displayID = table[i];
			
			if(CGDisplayIsMain(displayID))
			{
				CFArrayRef array = CGDisplayCopyAllDisplayModes(displayID, 0);
				CFIndex count = CFArrayGetCount(array);
				
				for(machine_uint i=0; i<count; i++)
				{
					CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(array, i);
					if(CFGetTypeID(mode) == CGDisplayModeGetTypeID())
					{
						CFStringRef encoding = CGDisplayModeCopyPixelEncoding(mode);
						
						if(CFStringCompare(encoding, CFSTR(IO32BitDirectPixels), 0) == kCFCompareEqualTo)
						{
							uint32 width  = (uint32)CGDisplayModeGetPixelWidth(mode);
							uint32 height = (uint32)CGDisplayModeGetPixelHeight(mode);
							
							if(width >= 1024 && height >= 768)
							{
								_configurations.emplace_back(WindowConfiguration(mode));
							}
						}
						
						CFRelease(encoding);
					}
				}
				
				break;
			}
		}
		
		delete[] table;
		
		_nativeWindow = 0;
#endif
		
#if RN_PLATFORM_LINUX
		_dpy = _context->_dpy;
		_win = _context->_win;
		
		_screenConfig = XRRGetScreenInfo(_dpy, _win);
		_originalSize = XRRConfigCurrentConfiguration(_screenConfig, &_originalRotation);
		
		int32 mainScreen = DefaultScreen(_dpy);
		int count;
		
		const XRRScreenSize *sizeArray = XRRSizes(_dpy, mainScreen, &count);
		for(uint32 i=0; i<count; i++)
		{
			uint32 width = sizeArray[i].width;
			uint32 height = sizeArray[i].height;
			
			if(width >= 1024 && height >= 768)
			{
				WindowConfiguration *configuration = new WindowConfiguration(i, width, height);
				_configurations.AddObject(configuration);
			}
		}
		
		char bitmap[8] = {0};
		XColor color = {0};
		
		Pixmap sourcePixmap = XCreateBitmapFromData(_dpy, _win, bitmap, 8, 8);
		Pixmap maskPixmap = XCreateBitmapFromData(_dpy, _win, bitmap, 8, 8);
		
		_emptyCursor = XCreatePixmapCursor(_dpy, sourcePixmap, maskPixmap, &color, &color, 0, 0);
		
		XFreePixmap(_dpy, maskPixmap);
		XFreePixmap(_dpy, sourcePixmap);
#endif
		
		std::sort(_configurations.begin(), _configurations.end(), [](const WindowConfiguration& a, const WindowConfiguration& b) {
			if(a.Width() < b.Width())
				return true;
			
			if(a.Width() == b.Width() && a.Height() < b.Height())
				return true;
			
			return false;
		});

		SetTitle("");
		SetConfiguration(_configurations.front(), _mask);
	}

	Window::~Window()
	{
#if RN_PLATFORM_MAC_OS
		[(RNNativeWindow *)_nativeWindow release];
#endif

#if RN_PLATFORM_LINUX
		XRRFreeScreenConfigInfo(_screenConfig);
#endif
	}

	void Window::SetTitle(const std::string& title)
	{
		_title = title;
		
#if RN_PLATFORM_MAC_OS
		[(RNNativeWindow *)_nativeWindow setTitle:[NSString stringWithUTF8String:title.c_str()]];
#endif
		
#if RN_PLATFORM_LINUX
		XStoreName(_dpy, _win, title.c_str());	
#endif
	}
	
	void Window::SetConfiguration(const WindowConfiguration& configuration, WindowMask mask)
	{
		uint32 width  = configuration.Width();
		uint32 height = configuration.Height();
		
		Renderer *renderer = Renderer::SharedInstance();
		
#if RN_PLATFORM_MAC_OS
		[(RNNativeWindow *)_nativeWindow release];
		
		if(mask & WindowMaskFullscreen)
		{
			NSRect displayRect = [[NSScreen mainScreen] frame];
			
			_nativeWindow = [[RNNativeWindow alloc] initWithFrame:displayRect andStyleMask:NSBorderlessWindowMask];
			[(RNNativeWindow *)_nativeWindow setLevel:NSMainMenuWindowLevel + 1];
			[(RNNativeWindow *)_nativeWindow setBackgroundColor:[NSColor blackColor]];
			[(RNNativeWindow *)_nativeWindow setOpaque:YES];
			[(RNNativeWindow *)_nativeWindow setHidesOnDeactivate:YES];
			
			renderer->SetDefaultFrame(displayRect.size.width, displayRect.size.height);
		}
		else
		{
			NSUInteger windowStyleMask = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
			_nativeWindow = [[RNNativeWindow alloc] initWithFrame:NSMakeRect(0, 0, configuration.Width(), configuration.Height()) andStyleMask:windowStyleMask];
			
			[(RNNativeWindow *)_nativeWindow center];
			
			renderer->SetDefaultFrame(width, height);
		}
		
		[(RNNativeWindow *)_nativeWindow setReleasedWhenClosed:NO];
		[(RNNativeWindow *)_nativeWindow setAcceptsMouseMovedEvents:YES];
		[(RNNativeWindow *)_nativeWindow setOpenGLContext:(NSOpenGLContext *)_context->_oglContext andPixelFormat:(NSOpenGLPixelFormat *)_context->_oglPixelFormat];
		
		[(RNNativeWindow *)_nativeWindow makeKeyAndOrderFront:nil];
		
		GLint sync = (mask & WindowMaskVSync) ? 1 : 0;
		[(NSOpenGLContext *)_context->_oglContext setValues:&sync forParameter:NSOpenGLCPSwapInterval];
#endif
		
#if RN_PLATFORM_LINUX
		XSetWindowAttributes windowAttributes;
		XID rootWindow = DefaultRootWindow(_dpy);
		
		XUnmapWindow(_dpy, _win);
		bool displayChanged = false;
		
		if(mask & WindowMaskFullscreen)
		{
			XRRSetScreenConfig(_win, _screenConfig, rootWindow, configuration->_modeIndex, RR_Rotate_0, CurrentTime);
			XMoveResizeWindow(_dpy, _win, 0, 0, width, height);
			
			windowAttributes.override_redirect = true;
			XChangeWindowAttributes(_dpy, _win, CWOverrideRedirect, &windowAttributes);
		}
		else
		{
			XRRSetScreenConfig(_dpy, _screenConfig, rootWindow, _originalSize, _originalRotation, CurrentTime);
				
			windowAttributes.override_redirect = false;
			XChangeWindowAttributes(_dpy, _win, CWOverrideRedirect, &windowAttributes);
			
			Screen *screen = DefaultScreenOfDisplay(_dpy);
			
			uint32 originX = (WidthOfScreen(screen) / 2) - (width / 2);
			uint32 originY = (HeightOfScreen(screen) / 2) - (height / 2);
			
			XMoveResizeWindow(_dpy, _win, originX, originY, width, height);
		}
		
		renderer->SetDefaultFrame(width, height);
		
		XSizeHints *sizeHints = XAllocSizeHints();
		
		sizeHints->flags = PMinSize | PMaxSize;
		sizeHints->min_width  = width;
		sizeHints->min_height = height;
		sizeHints->max_width  = width;
		sizeHints->max_height = height;
		XSetWMNormalHints(_dpy, _win, sizeHints);
		XFree(sizeHints);
		
		XMapRaised(_dpy, _win);
		
		if(mask & WindowMaskFullscreen)
		{
			XSetInputFocus(_dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
		}
#endif
		
		_mask = mask;
		_activeConfiguration = configuration;
		
		SetTitle(_title);
	}

	Rect Window::Frame() const
	{
		return Rect(0, 0, _activeConfiguration.Width(), _activeConfiguration.Height());
	}

	void Window::ShowCursor()
	{
		if(_cursorVisible)
			return;
		
		_cursorVisible = true;
		
#if RN_PLATFORM_MAC_OS
		[NSCursor unhide];
#endif
		
#if RN_PLATFORM_LINUX
		XUndefineCursor(_dpy, _win);
#endif
	}
	
	void Window::HideCursor()
	{
#if RN_PLATFORM_MAC_OS
		[NSCursor hide];
#endif
		
#if RN_PLATFORM_LINUX
		XDefineCursor(_dpy, _win, _emptyCursor);
#endif
	}
}



#if RN_PLATFORM_WINDOWS

void RNRegisterWindow();

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

#endif // RN_PLATFORM_WINDOWS
