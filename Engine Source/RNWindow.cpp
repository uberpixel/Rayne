//
//  RNWindow.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWindow.h"
#include "RNBaseInternal.h"
#include "RNContextInternal.h"
#include "RNWindowInternal.h"

#include "RNFile.h"
#include "RNTexture.h"
#include "RNKernel.h"
#include "RNSettings.h"
#include "RNInput.h"
#include "RNUIServer.h"
#include "RNLogging.h"

namespace RN
{
	RNDeclareMeta(WindowConfiguration)
	RNDeclareSingleton(Window)
	
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
	
#if RN_PLATFORM_MAC_OS
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
#endif
	
#if RN_PLATFORM_WINDOWS
	Screen::Screen(const char *name) :
		_display(name),
		_scaleFactor(1.0f)
	{
		for(DWORD modeNum = 0;; modeNum ++)
		{
			DEVMODE mode;
			
			mode.dmSize = sizeof(DEVMODE);
			mode.dmDriverExtra = 0;
			
			if(!EnumDisplaySettingsA(name, modeNum, &mode))
				break;
			
			uint32 width  = mode.dmPelsWidth;
			uint32 height = mode.dmPelsHeight;
			
			if((width >= 1024) && (height >= 768) && (mode.dmBitsPerPel >= 32))
			{
				WindowConfiguration *configuration = new WindowConfiguration(width, height, this);
				_configurations.AddObject(configuration->Autorelease());
			}
		}
#endif
		
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
		_internals->nativeWindow = nil;
#endif
		
#if RN_PLATFORM_WINDOWS
		for(DWORD devNum = 0;; devNum ++)
		{
			DISPLAY_DEVICEA device;
			
			device.cb = sizeof(DISPLAY_DEVICEA);
			if(!EnumDisplayDevicesA(nullptr, devNum, &device, 0))
				break;
			
			Screen *screen = new Screen(device.DeviceName);
			
			if(device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
			{
				_screens.insert(_screens.begin(), screen);
				_mainScreen = screen;
			}
			else
				_screens.push_back(screen);
		}
		
		_internals->hWnd = nullptr;
		_internals->displayChanged = false;
		_internals->context = nullptr;
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

		RN_ASSERT(_mainScreen, "A main screen is required for Rayne to work!");
		RN_ASSERT(!_screens.empty(), "There needs to be at least one screen!");

		bool failed = false;
		
		try
		{
			SetTitle("");
			
			Dictionary *screenConfig = Settings::GetSharedInstance()->GetObjectForKey<Dictionary>(kRNSettingsScreenKey);
			if(screenConfig)
			{
				Number *width  = screenConfig->GetObjectForKey<Number>(RNCSTR("width"));
				Number *height = screenConfig->GetObjectForKey<Number>(RNCSTR("height"));
				
				WindowConfiguration *temp = new WindowConfiguration(width->GetUint32Value(), height->GetUint32Value(), _mainScreen);
				SetConfiguration(temp->Autorelease(), _mask);
			}
			else
				failed = true;
		}
		catch(Exception e)
		{
			failed = true;
		}
		
		if(failed)
			SetConfiguration(_mainScreen->GetConfigurations().GetObjectAtIndex<WindowConfiguration>(0), _mask);
	}

	Window::~Window()
	{
#if RN_PLATFORM_MAC_OS
		[_internals->nativeWindow release];
#endif

#if RN_PLATFORM_WINDOWS
		if(_internals->hWnd)
			::DestroyWindow(_internals->hWnd);
		if(_internals->displayChanged)
			::ChangeDisplaySettingsA(nullptr, 0);
#endif

#if RN_PLATFORM_LINUX
		XRRFreeScreenConfigInfo(_internals->screenConfig);
#endif
		
		for(Screen *screen : _screens)
			delete screen;
		
		_activeConfiguration->Release();
	}

	void Window::SetTitle(const std::string& title)
	{
		_title = title;
		
#if RN_PLATFORM_MAC_OS
		[_internals->nativeWindow setTitle:[NSString stringWithUTF8String:title.c_str()]];
#endif
		
#if RN_PLATFORM_WINDOWS
		::SetWindowTextA(_kernel->GetMainWindow(), title.c_str());
#endif

#if RN_PLATFORM_LINUX
		XStoreName(_internals->dpy, _internals->win, title.c_str());
#endif
	}
	
	void Window::SetConfiguration(const WindowConfiguration *tconfiguration, Mask mask)
	{
		if(tconfiguration->IsEqual(_activeConfiguration) && mask == _mask)
			return;
		
		WindowConfiguration *configuration = const_cast<WindowConfiguration *>(tconfiguration);
		Renderer *renderer = Renderer::GetSharedInstance();
		
		Screen *screen = configuration->GetScreen();
		
		uint32 width  = configuration->GetWidth();
		uint32 height = configuration->GetHeight();
		
		RNDebug("Switching to {%i, %i} mask: 0x%x", static_cast<int>(width), static_cast<int>(height), mask);
		
#if RN_PLATFORM_MAC_OS
		[_context->_internals->context clearDrawable];
		[_internals->nativeWindow close];
		[_internals->nativeWindow release];
		
		[NSOpenGLContext clearCurrentContext];
		[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.00001f]];
		
		if(mask & MaskFullscreen)
		{
			const Rect& rect = screen->GetFrame();
			
			_internals->nativeWindow = [[RNNativeWindow alloc] initWithFrame:NSMakeRect(rect.x, rect.y, rect.width, rect.height) andStyleMask:NSBorderlessWindowMask];
			[_internals->nativeWindow setLevel:NSMainMenuWindowLevel + 1];
			[_internals->nativeWindow setBackgroundColor:[NSColor blackColor]];
			[_internals->nativeWindow setOpaque:YES];
			[_internals->nativeWindow setHidesOnDeactivate:YES];
			
			renderer->SetDefaultFrame(rect.width, rect.height);
			renderer->SetDefaultFactor(rect.width / width, rect.height / height);
		}
		else
		{
			NSUInteger windowStyleMask = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
			
			uint32 x = screen->GetFrame().x + ((screen->GetWidth()  / 2.0f) - (width / 2.0f));
			uint32 y = screen->GetFrame().y + ((screen->GetHeight() / 2.0f) - (height / 2.0f));
			
			_internals->nativeWindow = [[RNNativeWindow alloc] initWithFrame:NSMakeRect(x, y, width, height) andStyleMask:windowStyleMask];
			
			renderer->SetDefaultFrame(width, height);
			renderer->SetDefaultFactor(1.0f, 1.0f);
		}
		
		renderer->SetDefaultFBO(0);
		
		[_internals->nativeWindow setReleasedWhenClosed:NO];
		[_internals->nativeWindow setAcceptsMouseMovedEvents:YES];
		[_internals->nativeWindow setOpenGLContext:_context->_internals->context andPixelFormat:_context->_internals->pixelFormat];
		
		[_internals->nativeWindow makeKeyAndOrderFront:nil];
		[_context->_internals->context makeCurrentContext];
		
		GLint sync = (mask & MaskVSync) ? 1 : 0;
		[_context->_internals->context setValues:&sync forParameter:NSOpenGLCPSwapInterval];
		[_context->_internals->context update];
		
		[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.00001f]];
		
#endif

#if RN_PLATFORM_WINDOWS
		if(_internals->hWnd)
		{
			::DestroyWindow(_internals->hWnd);
			_internals->hWnd = nullptr;
		}
		
		HWND mainWindow = _kernel->GetMainWindow();

		if(mask & MaskFullscreen)
		{
			DEVMODE	mode = { 0 };

			mode.dmSize = sizeof(DEVMODE);
			mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
			mode.dmBitsPerPel = 32;
			mode.dmPelsWidth = width;
			mode.dmPelsHeight = height;

			if(::ChangeDisplaySettingsA(&mode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
				throw Exception(Exception::Type::InconsistencyException, "Failed to switch configuration!");

			_internals->displayChanged = true;

			::SetWindowPos(mainWindow, HWND_NOTOPMOST, 0, 0, width, height, SWP_NOCOPYBITS);
			::SetWindowLongPtrA(mainWindow, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN);
			::SetWindowPos(mainWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

			renderer->SetDefaultFrame(width, height);
			renderer->SetDefaultFactor(1.0f, 1.0f);
		}
		else
		{
			if(_internals->displayChanged)
			{
				::ChangeDisplaySettingsA(nullptr, 0);
				_internals->displayChanged = false;
			}

			RECT windowRect;

			windowRect.left = (GetSystemMetrics(SM_CXFULLSCREEN) / 2) - (width / 2);
			windowRect.top  = (GetSystemMetrics(SM_CYFULLSCREEN) / 2) - (height / 2);
			windowRect.right  = windowRect.left + width;
			windowRect.bottom = windowRect.top + height;

			::AdjustWindowRectEx(&windowRect, WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, false, 0);

			::SetWindowPos(mainWindow, HWND_NOTOPMOST, windowRect.left, windowRect.top, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, SWP_NOCOPYBITS);
			::SetWindowLongPtrA(mainWindow, GWL_STYLE, WS_CLIPCHILDREN | WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
			::SetWindowPos(mainWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
			
			renderer->SetDefaultFrame(width, height);
			renderer->SetDefaultFactor(1.0f, 1.0f);
		}

		
		_internals->hWnd = ::CreateWindowExW(0, L"RNWindowClass", L"", WS_CHILD | WS_VISIBLE, 0, 0, width, height, mainWindow, nullptr, _kernel->GetInstance(), nullptr);
		_internals->hDC = ::GetDC(_internals->hWnd);

		if(_internals->context)
		{
			_internals->context->Release();
			_internals->context = nullptr;
		}

		_internals->context = new Context(_kernel->GetContext(), _internals->hWnd);
		_internals->context->MakeActiveContext();

		if(wgl::SwapIntervalEXT)
		{
			GLint interval = (mask & MaskVSync) ? 1 : 0;
			wgl::SwapIntervalEXT(interval);
		}
	
		renderer->SetDefaultFBO(0);
#endif
		
#if RN_PLATFORM_LINUX
		XSetWindowAttributes windowAttributes;
		XID rootWindow = DefaultRootWindow(_internals->dpy);
		
		XUnmapWindow(_internals->dpy, _internals->win);
		bool displayChanged = false;
		
		if(mask & MaskFullscreen)
		{
			XRRSetScreenConfig(_internals->win, _internals->screenConfig, rootWindow, configuration->_modeIndex, RR_Rotate_0, CurrentTime);
			XMoveResizeWindow(_internals->dpy, _internals->win, 0, 0, width, height);
			
			windowAttributes.override_redirect = true;
			XChangeWindowAttributes(_internals->dpy, _internals->win, CWOverrideRedirect, &windowAttributes);
		}
		else
		{
			XRRSetScreenConfig(_internals->dpy, _internals->screenConfig, rootWindow, _internals->originalSize, _internals->originalRotation, CurrentTime);
				
			windowAttributes.override_redirect = false;
			XChangeWindowAttributes(_internals->dpy, _internals->win, CWOverrideRedirect, &windowAttributes);
			
			Screen *screen = DefaultScreenOfDisplay(_internals->dpy);
			
			uint32 originX = (WidthOfScreen(screen) / 2) - (width / 2);
			uint32 originY = (HeightOfScreen(screen) / 2) - (height / 2);
			
			XMoveResizeWindow(_internals->dpy, _internals->win, originX, originY, width, height);
		}
		
		renderer->SetDefaultFrame(width, height);
		
		XSizeHints *sizeHints = XAllocSizeHints();
		
		sizeHints->flags = PMinSize | PMaxSize;
		sizeHints->min_width  = width;
		sizeHints->min_height = height;
		sizeHints->max_width  = width;
		sizeHints->max_height = height;
		XSetWMNormalHints(_internals->dpy, _internals->win, sizeHints);
		XFree(sizeHints);
		
		XMapRaised(_internals->dpy, _internals->win);
		
		if(mask & MaskFullscreen)
			XSetInputFocus(_internals->dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
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
#if RN_PLATFORM_WINDOWS
		::ShowCursor(true);
#endif
#if RN_PLATFORM_LINUX
		XUndefineCursor(_internals->dpy, _internals->win);
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
#if RN_PLATFORM_WINDOWS
		::ShowCursor(false);
#endif
#if RN_PLATFORM_LINUX
		XDefineCursor(_internals->dpy, _internals->win, _internals->emptyCursor);
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
