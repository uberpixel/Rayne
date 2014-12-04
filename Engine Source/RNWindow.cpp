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
#include "RNOpenGLQueue.h"
#include "RNFile.h"
#include "RNTexture.h"
#include "RNKernel.h"
#include "RNSettings.h"
#include "RNInput.h"
#include "RNUIServer.h"
#include "RNLogging.h"

namespace RN
{
	RNDefineMeta(WindowConfiguration, Object)
	RNDefineSingleton(Window)
	
	// ---------------------
	// MARK: -
	// MARK: WindowConfiguration
	// ---------------------
	
	WindowConfiguration::WindowConfiguration(const WindowConfiguration *other) :
		WindowConfiguration(other->_width, other->_height, other->_screen)
	{}

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
					
					if(width >= 320 && height >= 240)
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
	Screen::Screen(HMONITOR monitor) :
		_monitor(monitor),
		_scaleFactor(1.0f),
		_isMain(false)
	{
		MONITORINFOEXA  info;
		info.cbSize = sizeof(MONITORINFOEXA);

		if(!::GetMonitorInfoA(_monitor, &info))
			throw Exception(Exception::Type::GenericException, "GetMonitorInfoA failed");

		_isMain = (info.dwFlags & MONITORINFOF_PRIMARY);

		_frame.x = info.rcMonitor.left;
		_frame.y = info.rcMonitor.top;

		_frame.width  = info.rcMonitor.right - info.rcMonitor.left;
		_frame.height = info.rcMonitor.bottom - info.rcMonitor.top;

		for(DWORD modeNum = 0;; modeNum ++)
		{
			DEVMODE mode;
			
			mode.dmSize = sizeof(DEVMODE);
			mode.dmDriverExtra = 0;
			
			if(!EnumDisplaySettingsA(info.szDevice, modeNum, &mode))
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

		if(_configurations.GetCount() == 0)
			throw Exception(Exception::Type::GenericException, "No window configurations found for monitor");
		
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

#if RN_PLATFORM_WINDOWS
	static std::vector<HMONITOR> __MonitorHandles;

	static BOOL __MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
	{
		__MonitorHandles.push_back(hMonitor);
		return true;
	}
#endif
		
	Window::Window()
	{
		_kernel = Kernel::GetSharedInstance();
		_internals->context = nullptr;
		
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
		
		delete [] table;
		_internals->nativeWindow = nil;
#endif
		
#if RN_PLATFORM_WINDOWS
		::EnumDisplayMonitors(nullptr, nullptr, (MONITORENUMPROC)__MonitorEnumProc, NULL);

		for(HMONITOR monitor : __MonitorHandles)
		{
			try
			{
				Screen *screen = new Screen(monitor);

				if(screen->IsMainScreen())
				{
					_screens.insert(_screens.begin(), screen);
					_mainScreen = screen;
				}
				else
					_screens.push_back(screen);
			}
			catch(Exception e)
			{}
		}
		
		_internals->hWnd = nullptr;
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
			
			Mask mask = _mask;
			
			Dictionary *screenConfig = Settings::GetSharedInstance()->GetObjectForKey<Dictionary>(kRNSettingsScreenKey);
			if(screenConfig)
			{
				Number *width  = screenConfig->GetObjectForKey<Number>(RNCSTR("width"));
				Number *height = screenConfig->GetObjectForKey<Number>(RNCSTR("height"));
				
				Number *fullscreen = screenConfig->GetObjectForKey<Number>(RNCSTR("fullscreen"));
				if(fullscreen)
					mask |= (fullscreen->GetBoolValue()) ? Mask::Fullscreen : 0;
				
				Number *borderless = screenConfig->GetObjectForKey<Number>(RNCSTR("borderless"));
				if(borderless)
					mask |= (borderless->GetBoolValue()) ? Mask::Borderless : 0;
				
				WindowConfiguration *temp = new WindowConfiguration(width->GetUint32Value(), height->GetUint32Value(), _mainScreen);
				ActivateConfiguration(temp->Autorelease(), mask);
			}
			else
				failed = true;
		}
		catch(Exception e)
		{
			failed = true;
		}
		
		if(failed)
			ActivateConfiguration(_mainScreen->GetConfigurations()->GetObjectAtIndex<WindowConfiguration>(0), _mask);
	}

	Window::~Window()
	{
#if RN_PLATFORM_MAC_OS
		[_internals->nativeWindow release];
#endif

#if RN_PLATFORM_WINDOWS
		if(_internals->hWnd)
			::DestroyWindow(_internals->hWnd);
#endif

#if RN_PLATFORM_LINUX
		XRRFreeScreenConfigInfo(_internals->screenConfig);
#endif
		
		for(Screen *screen : _screens)
			delete screen;
		
		_activeConfiguration->Release();
	}

	void Window::SetTitle(const std::string &title)
	{
		if(!Thread::GetMainThread()->OnThread())
		{
			Kernel::GetSharedInstance()->ScheduleFunction([this, title] {
				SetTitle(title);
			});

			return;
		}

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

	void Window::SetPosition(const Vector2 &position)
	{
		if(!Thread::GetMainThread()->OnThread())
		{
			Kernel::GetSharedInstance()->ScheduleFunction([this, position] {
				SetPosition(position);
			});

			return;
		}
		
#if RN_PLATFORM_MAC_OS
		NSScreen *screen = [_internals->nativeWindow screen];
		NSPoint point = NSMakePoint(position.x, position.y);
		
		point.y = [screen frame].size.height - position.y;
		
		[_internals->nativeWindow setFrameTopLeftPoint:point];
#endif

#if RN_PLATFORM_WINDOWS
		HWND mainWindow = _kernel->GetMainWindow();
		RECT rect;

		::GetWindowRect(mainWindow, &rect);
		::AdjustWindowRectEx(&rect, _internals->style, false, 0);


		uint32 width  = rect.right - rect.left;
		uint32 height = rect.bottom - rect.top;

		rect.left = position.x;
		rect.top  = position.y;
		
		::SetWindowPos(mainWindow, HWND_NOTOPMOST, rect.left, rect.top, width, height, SWP_NOCOPYBITS);
		::SetWindowPos(mainWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
#endif
	}

	void Window::ActivateConfiguration(WindowConfiguration *configuration, Mask mask)
	{
		if(!Thread::GetMainThread()->OnThread())
		{
			configuration = configuration->Copy();

			Kernel::GetSharedInstance()->ScheduleFunction([this, configuration, mask] {
				ActivateConfiguration(configuration, mask);
				configuration->Release();
			});

			return;
		}


		if(configuration->IsEqual(_activeConfiguration) && mask == _mask)
			return;
		
		Renderer *renderer = Renderer::GetSharedInstance();
		Screen *screen = configuration->GetScreen();
		
		uint32 width  = configuration->GetWidth();
		uint32 height = configuration->GetHeight();
		
		RNDebug("Switching to {%i, %i} mask: 0x%x", static_cast<int>(width), static_cast<int>(height), mask);
		
#if RN_PLATFORM_MAC_OS
		if(_internals->nativeWindow)
		{
			[_internals->context->_internals->context clearDrawable];
			[_internals->nativeWindow close];
			[_internals->nativeWindow release];
			
			[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.00001f]];
		}
		
		if(mask & Mask::Fullscreen)
		{
			const Rect &rect = screen->GetFrame();
			
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
			
			if(mask & Mask::Borderless)
				windowStyleMask = NSBorderlessWindowMask;
			
			uint32 x = screen->GetFrame().x + ((screen->GetWidth()  / 2.0f) - (width / 2.0f));
			uint32 y = screen->GetFrame().y + ((screen->GetHeight() / 2.0f) - (height / 2.0f));
			
			_internals->nativeWindow = [[RNNativeWindow alloc] initWithFrame:NSMakeRect(x, y, width, height) andStyleMask:windowStyleMask];
			
			renderer->SetDefaultFrame(width, height);
			renderer->SetDefaultFactor(1.0f, 1.0f);
		}
		
		renderer->SetDefaultFBO(0);
		
		_internals->context = new Context(_kernel->GetContext());
		_internals->context->MakeActiveContext();
		
		// Get the window ready
		[_internals->nativeWindow setReleasedWhenClosed:NO];
		[_internals->nativeWindow setAcceptsMouseMovedEvents:YES];
		[_internals->nativeWindow setOpenGLContext:_internals->context->_internals->context
									andPixelFormat:_internals->context->_internals->pixelFormat];
		
		[_internals->nativeWindow makeKeyAndOrderFront:nil];
		
		
		// Update the context and hand it over to the OpenGLQUeue
		GLint sync = (mask & Mask::VSync) ? 1 : 0;
		[_internals->context->_internals->context setValues:&sync forParameter:NSOpenGLCPSwapInterval];
		[_internals->context->_internals->context update];
		
		[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.00001f]];
		
		_internals->context->DeactivateContext();
		OpenGLQueue::GetSharedInstance()->SwitchContext(_internals->context);
#endif

#if RN_PLATFORM_WINDOWS
		if(_internals->hWnd)
		{
			::DestroyWindow(_internals->hWnd);
			_internals->hWnd = nullptr;
		}
		
		HWND mainWindow = _kernel->GetMainWindow();
		RECT monitorRect;

		{
			const Rect &rect = screen->GetFrame();

			monitorRect.left = rect.x;
			monitorRect.top  = rect.y;

			monitorRect.right  = rect.width - rect.x;
			monitorRect.bottom = rect.height - rect.y;
		}

		if(mask & Mask::Fullscreen)
		{
			DWORD windowStyle = WS_CLIPCHILDREN |  WS_SYSMENU;
			const Rect &rect = screen->GetFrame();

			::SetWindowPos(mainWindow, HWND_NOTOPMOST, monitorRect.left, monitorRect.top, rect.width, rect.height, SWP_NOCOPYBITS);
			::SetWindowLongPtrA(mainWindow, GWL_STYLE, windowStyle);
			::SetWindowPos(mainWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

			renderer->SetDefaultFrame(rect.width, rect.height);
			renderer->SetDefaultFactor(rect.width / width, rect.height / height);

			width  = rect.width;
			height = rect.height;

			_internals->style = windowStyle;
		}
		else
		{
			DWORD windowStyle = WS_CLIPCHILDREN | WS_SYSMENU;
			if(!(mask & Mask::Borderless))
				windowStyle |= WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX;

			uint32 x = monitorRect.left + ((screen->GetWidth() / 2.0f) - (width / 2.0f));
			uint32 y = monitorRect.top  + ((screen->GetHeight() / 2.0f) - (height / 2.0f));

			RECT windowRect;
			windowRect.left   = x;
			windowRect.top    = y;
			windowRect.right  = x + width;
			windowRect.bottom = y + height;

			::AdjustWindowRectEx(&windowRect, windowStyle, false, 0);

			::SetWindowPos(mainWindow, HWND_NOTOPMOST, windowRect.left, windowRect.top, width, height, SWP_NOCOPYBITS);
			::SetWindowLongPtrA(mainWindow, GWL_STYLE, windowStyle);
			::SetWindowPos(mainWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
			
			renderer->SetDefaultFrame(width, height);
			renderer->SetDefaultFactor(1.0f, 1.0f);

			_internals->style = windowStyle;
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
			GLint interval = (mask & Mask::VSync) ? 1 : 0;
			wgl::SwapIntervalEXT(interval);
		}
	
		_internals->context->DeactivateContext();

		renderer->SetDefaultFBO(0);
		OpenGLQueue::GetSharedInstance()->SwitchContext(_internals->context);
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

	Vector2 Window::GetSize() const
	{
		return Vector2(_activeConfiguration->GetWidth(), _activeConfiguration->GetHeight());
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
		
	void Window::Flush()
	{
		if(_flushProc)
		{
			_flushProc();
			return;
		}
		
#if RN_PLATFORM_MAC_OS
		CGLFlushDrawable(_internals->context->_internals->cglContext);
#endif
		
#if RN_PLATFORM_LINUX
		glXSwapBuffers(_context->_dpy, _context->_win);
#endif
		
#if RN_PLATFORM_WINDOWS
		::SwapBuffers(_internals->hDC);
#endif
	}
		
	void Window::SetFlushProc(std::function<void()> flush)
	{
		_flushProc = flush;
	}

#if RN_PLATFORM_WINDOWS
	HWND Window::GetCurrentWindow() const
	{
		return _internals->hWnd;
	}

	HDC Window::GetCurrentDC() const
	{
		return _internals->hDC;
	}
#endif
}
