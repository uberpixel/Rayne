//
//  RNScreen.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Base/RNBaseInternal.h"
#include "../Base/RNKernel.h"
#include "RNScreen.h"

namespace RN
{
	RNDefineMeta(Screen, Object)

#if RN_PLATFORM_WINDOWS
	static std::vector<HMONITOR> __MonitorHandles;

	static BOOL __MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
	{
		__MonitorHandles.push_back(hMonitor);
		return true;
	}
#endif

	static Screen *_mainScreen;
	static Array *_screens;

	Screen *Screen::GetMainScreen()
	{
		return _mainScreen;
	}

	Array *Screen::GetScreens()
	{
		return _screens;
	}

	void Screen::InitializeScreens()
	{
		_screens = new Array();

#if RN_PLATFORM_MAC_OS
		@autoreleasepool {

			CGDisplayCount count;

			CGGetActiveDisplayList(0, 0, &count);
			CGDirectDisplayID *table = new CGDirectDisplayID[count];

			CGGetActiveDisplayList(count, table, &count);
			for(size_t i = 0; i < count; i ++)
			{
				try
				{
					Screen *screen = new Screen(table[i]);
					_screens->AddObject(screen->Autorelease());

					if(screen->IsMainScreen())
						_mainScreen = screen;
				}
				catch(Exception &e) { }
			}

			delete [] table;
		}
#endif
#if RN_PLATFORM_WINDOWS

		::EnumDisplayMonitors(nullptr, nullptr, (MONITORENUMPROC)&__MonitorEnumProc, 0);

		for(HMONITOR monitor : __MonitorHandles)
		{
			try
			{
				Screen *screen = new Screen(monitor);

				_screens->AddObject(screen->Autorelease());

				if(screen->IsMainScreen())
					_mainScreen = screen;
			}
			catch(Exception e)
			{}
		}

		__MonitorHandles.clear();
#endif
#if RN_PLATFORM_LINUX

		const xcb_setup_t *setup = xcb_get_setup(Kernel::GetSharedInstance()->GetXCBConnection());

		xcb_screen_iterator_t iterator = xcb_setup_roots_iterator(setup);
		int count = xcb_setup_roots_length(setup);

		for(int i = 0; i < count; i ++)
		{
			xcb_screen_t *screen = iterator.data;

			Screen *temp = new Screen(screen, (i == 0));

			if(i == 0)
				_mainScreen = temp;

			_screens->AddObject(temp);
			temp->Release();

			xcb_screen_next(&iterator);
		}
#endif
	}
	void Screen::TeardownScreens()
	{
		_screens->Release();
		_mainScreen->Release();

		_screens = nullptr;
		_mainScreen = nullptr;
	}

#if RN_PLATFORM_MAC_OS
	Screen::Screen(CGDirectDisplayID display) :
		_resolutions(new Array()),
		_name(nullptr),
		_isMainScreen(CGDisplayIsMain(display)),
		_display(display),
		_nsscreen(nullptr)
	{
		// Find the NSScreen that corresponds to this screen
		NSArray *screens = [NSScreen screens];

		for(NSInteger i = 0; i < [screens count]; i ++)
		{
			NSScreen *screen = [screens objectAtIndex:i];
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
						_frame = Rect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
						_nsscreen = screen;
						break;
					}
				}

				if(_nsscreen)
					break;
			}
		}

		if(!_nsscreen)
			throw InconsistencyException("Couldn't find NSScreen matching CGDirectDisplayID");

		_scaleFactor = Kernel::GetSharedInstance()->GetScaleFactor();
		_scaleFactor = std::max(_scaleFactor, static_cast<float>([(NSScreen *)_nsscreen backingScaleFactor]));

		// Enumerate through all supported modes
		CFArrayRef array = CGDisplayCopyAllDisplayModes(display, 0);
		CFIndex count    = CFArrayGetCount(array);

		for(size_t i = 0; i < count; i ++)
		{
			CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(array, i);
			if(CFGetTypeID(mode) == CGDisplayModeGetTypeID())
			{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
				CFStringRef encoding = CGDisplayModeCopyPixelEncoding(mode);
#pragma clang diagnostic pop

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

						Value *value = Value::WithVector2(Vector2(width, height));
						_resolutions->AddObject(value);
					}
				}

				CFRelease(encoding);
			}
		}

		CFRelease(array);

		NSString *screenName = nil;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		NSDictionary *deviceInfo = (NSDictionary *)IODisplayCreateInfoDictionary(CGDisplayIOServicePort(display), kIODisplayOnlyPreferredName);
#pragma clang diagnostic pop
		
		NSDictionary *localizedNames = [deviceInfo objectForKey:[NSString stringWithUTF8String:kDisplayProductName]];

		if([localizedNames count] > 0)
			screenName = [localizedNames objectForKey:[[localizedNames allKeys] objectAtIndex:0]];

		if(screenName)
			_name = RNSTR([screenName UTF8String])->Retain();

		[deviceInfo release];
	}
#endif

#if RN_PLATFORM_WINDOWS
	Screen::Screen(HMONITOR monitor) :
		_resolutions(new Array()),
		_name(nullptr),
		_monitor(monitor),
		_scaleFactor(1.0f),
		_isMainScreen(false)
	{
		MONITORINFOEXA info;
		info.cbSize = sizeof(MONITORINFOEXA);

		if(!::GetMonitorInfoA(_monitor, &info))
			throw InconsistencyException("GetMonitorInfoA failed");

		_isMainScreen = (info.dwFlags & MONITORINFOF_PRIMARY);

		_frame.x = info.rcMonitor.left;
		_frame.y = info.rcMonitor.top;

		_frame.width = info.rcMonitor.right - info.rcMonitor.left;
		_frame.height = info.rcMonitor.bottom - info.rcMonitor.top;

		for(DWORD modeNum = 0; ; modeNum++)
		{
			DEVMODE mode;

			mode.dmSize = sizeof(DEVMODE);
			mode.dmDriverExtra = 0;

			if(!::EnumDisplaySettingsA(info.szDevice, modeNum, &mode))
				break;

			uint32 width = mode.dmPelsWidth;
			uint32 height = mode.dmPelsHeight;

			if(width >= 320 && height >= 240)
			{
				Value *value = Value::WithVector2(Vector2(width, height));
				_resolutions->AddObject(value);
			}
		}

		_name = RNSTR(info.szDevice)->Retain();
	}
#endif
#if RN_PLATFORM_LINUX
	Screen::Screen(xcb_screen_t *screen, bool mainScreen) :
		_scaleFactor(1.0),
		_resolutions(new Array()),
		_isMainScreen(mainScreen)
	{
		std::memcpy(&_screen, screen, sizeof(xcb_screen_t));

		xcb_connection_t *connection = Kernel::GetSharedInstance()->GetXCBConnection();
		xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(connection, xcb_get_geometry(connection, _screen.root), nullptr);

		_frame.x = geometry->x;
		_frame.y = geometry->y;
		_frame.width = geometry->width;
		_frame.height = geometry->height;

		free(geometry);

		_name = RNSTR("")->Retain();

		Value *value = Value::WithVector2(_frame.GetSize());
		_resolutions->AddObject(value);
	}
#endif

	Screen::~Screen()
	{
		if(_name)
			_name->Release();

		_resolutions->Release();
	}
}
