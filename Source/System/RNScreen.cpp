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
#if RN_PLATFORM_MAC_OS
		@autoreleasepool {

			_screens = new Array();

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
		_display(display)
	{
		// Find the NSScreen that corresponds to this screen
		NSArray *screens = [NSScreen screens];
		NSScreen *thisScreen = nil;

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
			throw InconsistencyException("Couldn't find NSScreen matching CGDirectDisplayID");

		_scaleFactor = Kernel::GetSharedInstance()->GetScaleFactor();
		_scaleFactor = std::max(_scaleFactor, static_cast<float>([thisScreen backingScaleFactor]));

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
	}

	Screen::~Screen()
	{
		if(_name)
			_name->Release();

		_resolutions->Release();
	}
#endif
}
