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
	RN::Kernel::GetSharedInstance()->Exit();
	return NO;
}

- (void)keyDown:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)keyUp:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)mouseMoved:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)mouseDown:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)otherMouseDragged:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)mouseUp:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)scrollWheel:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}


- (BOOL)canBecomeKeyWindow
{
	return YES;
}

- (BOOL)canBecomeMainWindow
{
	return YES;
}


- (void)windowDidBecomeKey:(NSNotification *)notification
{
	RN::Input::GetSharedInstance()->InvalidateMouse();
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

		RN::Input::GetSharedInstance()->HandleTouchEvent(temp);
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

		RN::Input::GetSharedInstance()->HandleTouchEvent(temp);
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

		RN::Input::GetSharedInstance()->HandleTouchEvent(temp);
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

		RN::Input::GetSharedInstance()->HandleTouchEvent(temp);
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

	float scaleFactor = RN::Kernel::GetSharedInstance()->ScaleFactor();

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

		[_renderLayer setContentsScale:RN::Kernel::GetSharedInstance()->ScaleFactor()];
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
	RNDeclareMeta(WindowConfiguration)
	
	// ---------------------
	// MARK: -
	// MARK: WindowConfiguration
	// ---------------------
	
	WindowConfiguration::WindowConfiguration(uint32 width, uint32 height) :
		WindowConfiguration(width, height, nullptr)
	{}
	
	WindowConfiguration::WindowConfiguration(uint32 width, uint32 height, Screen *screen)
	{
		_width  = width;
		_height = height;
		
		_screen = screen ? screen : Window::GetSharedInstance()->GetMainScreen();
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Screen
	// ---------------------
	
	Screen::Screen(CGDirectDisplayID display) :
		_display(display)
	{
		// Find the NSScreen that corresponds to this screen
		NSArray *screens = [NSScreen screens];
		NSScreen *thisScreen = nil;
		
		for(NSScreen *screen in screens)
		{
			NSRect frame = [screen frame];
			
			CGDirectDisplayID displayIDs[5];
			uint32 displayCount = 0;
			
			CGError error = CGGetDisplaysWithRect(NSRectToCGRect(frame), 5, displayIDs, &displayCount);
			if(error == kCGErrorSuccess)
			{
				for(uint32 i = 0; i < displayCount; i ++)
				{
					if(displayIDs[i] == display)
					{
						thisScreen = screen;
						
						_frame = Rect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
						_width  = _frame.width;
						_height = _frame.height;
						
						break;
					}
				}
				
				if(thisScreen)
					break;
			}
		}
		
		if(!thisScreen)
			throw Exception(Exception::Type::InconsistencyException, "Couldn't find NSScreen matching CGDirectDisplayID");
		
		_scaleFactor = Kernel::GetSharedInstance()->GetScaleFactor();
		_scaleFactor = std::min(_scaleFactor, static_cast<float>([thisScreen backingScaleFactor]));
		
		// Enumerate through all supported modes
		CFArrayRef array = CGDisplayCopyAllDisplayModes(display, 0);
		CFIndex count    = CFArrayGetCount(array);
		
		for(size_t i = 0; i < count; i ++)
		{
			CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(array, i);
			if(CFGetTypeID(mode) == CGDisplayModeGetTypeID())
			{
				CFStringRef encoding = CGDisplayModeCopyPixelEncoding(mode);
				
				if(CFStringCompare(encoding, CFSTR(IO32BitDirectPixels), 0) == kCFCompareEqualTo)
				{
					uint32 width  = static_cast<uint32>(CGDisplayModeGetPixelWidth(mode));
					uint32 height = static_cast<uint32>(CGDisplayModeGetPixelHeight(mode));
					
					if(width >= 1024 && height >= 768)
					{
						if(_scaleFactor >= 1.5f)
						{
							width  >>= 1;
							height >>= 1;
						}
						
						WindowConfiguration *configuration = new WindowConfiguration(width, height, this);
						_configurations.AddObject(configuration->Autorelease());
					}
				}
				
				CFRelease(encoding);
			}
		}
		
		CFRelease(array);
		
		_configurations.Sort<WindowConfiguration>([](const WindowConfiguration *left, const WindowConfiguration *right) {
			
			if(left->GetWidth() < right->GetWidth())
				return ComparisonResult::LessThan;
			
			if(left->GetWidth() == right->GetWidth() && left->GetHeight() < right->GetHeight())
				return ComparisonResult::LessThan;
			
			return ComparisonResult::GreaterThan;
		});
	}
	
	Screen::~Screen()
	{}
	
	// ---------------------
	// MARK: -
	// MARK: Window
	// ---------------------
	
	Window::Window()
	{
		_kernel  = Kernel::GetSharedInstance();
		_context = _kernel->GetContext();
		
		_mask = 0;
		_cursorVisible = true;
		_mouseCaptured = false;
		_activeScreen  = nullptr;
		_activeConfiguration = nullptr;
		
#if RN_PLATFORM_MAC_OS
		CGDisplayCount count;
		
		CGGetActiveDisplayList(0, 0, &count);
		CGDirectDisplayID *table = new CGDirectDisplayID[count];
		
		CGGetActiveDisplayList(count, table, &count);
		for(size_t i = 0; i < count; i ++)
		{
			try
			{
				CGDirectDisplayID displayID = table[i];
				Screen *screen = new Screen(displayID);
				
				_screens.push_back(screen);
				
				if(CGDisplayIsMain(displayID))
					_mainScreen = screen;
			}
			catch(Exception e)
			{}
		}
		
		delete[] table;
		_nativeWindow = nullptr;
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
		
		SetTitle("");
		SetConfiguration(_mainScreen->GetConfigurations().GetObjectAtIndex<WindowConfiguration>(0), _mask);
	}

	Window::~Window()
	{
#if RN_PLATFORM_MAC_OS
		[(RNNativeWindow *)_nativeWindow release];
#endif

#if RN_PLATFORM_LINUX
		XRRFreeScreenConfigInfo(_screenConfig);
#endif
		
		for(Screen *screen : _screens)
		{
			delete screen;
		}
		
		_activeConfiguration->Release();
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
	
	void Window::SetConfiguration(const WindowConfiguration *tconfiguration, WindowMask mask)
	{
		if(tconfiguration->IsEqual(_activeConfiguration))
			return;
		
		WindowConfiguration *configuration = const_cast<WindowConfiguration *>(tconfiguration);
		Renderer *renderer = Renderer::GetSharedInstance();
		
		Screen *screen = configuration->GetScreen();
		
		uint32 width  = configuration->GetWidth();
		uint32 height = configuration->GetHeight();
		
#if RN_PLATFORM_MAC_OS
		[(RNNativeWindow *)_nativeWindow release];
		
		if(mask & WindowMaskFullscreen)
		{
			const Rect& rect = screen->GetFrame();
			
			_nativeWindow = [[RNNativeWindow alloc] initWithFrame:NSMakeRect(rect.x, rect.y, rect.width, rect.height) andStyleMask:NSBorderlessWindowMask];
			[(RNNativeWindow *)_nativeWindow setLevel:NSMainMenuWindowLevel + 1];
			[(RNNativeWindow *)_nativeWindow setBackgroundColor:[NSColor blackColor]];
			[(RNNativeWindow *)_nativeWindow setOpaque:YES];
			[(RNNativeWindow *)_nativeWindow setHidesOnDeactivate:YES];
			
			renderer->SetDefaultFrame(rect.width, rect.height);
			renderer->SetDefaultFactor(rect.width / width, rect.height / height);
		}
		else
		{
			NSUInteger windowStyleMask = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
			
			uint32 x = screen->GetFrame().x + ((screen->GetWidth()  / 2.0f) - (width / 2.0f));
			uint32 y = screen->GetFrame().y + ((screen->GetHeight() / 2.0f) - (height / 2.0f));
			
			_nativeWindow = [[RNNativeWindow alloc] initWithFrame:NSMakeRect(x, y, width, height) andStyleMask:windowStyleMask];
			
			renderer->SetDefaultFrame(width, height);
			renderer->SetDefaultFactor(1.0f, 1.0f);
		}
		
		[(RNNativeWindow *)_nativeWindow setReleasedWhenClosed:NO];
		[(RNNativeWindow *)_nativeWindow setAcceptsMouseMovedEvents:YES];
		[(RNNativeWindow *)_nativeWindow setOpenGLContext:(NSOpenGLContext *)_context->_oglContext andPixelFormat:(NSOpenGLPixelFormat *)_context->_oglPixelFormat];
		
		[(RNNativeWindow *)_nativeWindow makeKeyAndOrderFront:nil];
		
		GLint sync = (mask & WindowMaskVSync) ? 1 : 0;
		[(NSOpenGLContext *)_context->_oglContext setValues:&sync forParameter:NSOpenGLCPSwapInterval];
		[(NSOpenGLContext *)_context->_oglContext update];
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
		
		bool screenChanged = (_activeScreen != screen);
		bool scaleChanged = (!_activeScreen || Math::FastAbs(_activeScreen->GetScaleFactor() - screen->GetScaleFactor()) > k::EpsilonFloat);
		
		_mask = mask;
		
		if(_activeConfiguration)
			_activeConfiguration->Release();
		
		_activeConfiguration = configuration->Retain();
		_activeScreen = screen;
		
		SetTitle(_title);
		
		MessageCenter::GetSharedInstance()->PostMessage(kRNWindowConfigurationChanged, nullptr, nullptr);
		
		if(screenChanged)
			MessageCenter::GetSharedInstance()->PostMessage(kRNWindowScreenChanged, nullptr, nullptr);
		
		if(scaleChanged)
			MessageCenter::GetSharedInstance()->PostMessage(kRNWindowScaleFactorChanged, nullptr, nullptr);
	}

	Rect Window::GetFrame() const
	{
		return Rect(0, 0, _activeConfiguration->GetWidth(), _activeConfiguration->GetHeight());
	}

#if RN_PLATFORM_MAC_OS
	Screen *Window::GetScreenWithID(CGDirectDisplayID display)
	{
		for(Screen *screen : _screens)
		{
			if(screen->_display == display)
				return screen;
		}
		
		return nullptr;
	}
#endif
	
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
		if(!_cursorVisible)
			return;
		
		_cursorVisible = false;
		
#if RN_PLATFORM_MAC_OS
		[NSCursor hide];
#endif
		
#if RN_PLATFORM_LINUX
		XDefineCursor(_dpy, _win, _emptyCursor);
#endif
	}
	
	void Window::CaptureMouse()
	{
		if(_mouseCaptured)
			return;
		
		_mouseCaptured = true;
		
#if RN_PLATFORM_MAC_OS
#endif
	}
	
	void Window::ReleaseMouse()
	{
		if(!_mouseCaptured)
			return;
		
		_mouseCaptured = false;
		
#if RN_PLATFORM_MAC_OS
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
				RN::Kernel::GetSharedInstance()->Exit();
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
