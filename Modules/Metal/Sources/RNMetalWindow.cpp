//
//  RNMetalWindow.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Metal/Metal.h>
#include "RNMetalWindow.h"
#include "RNMetalInternals.h"
#include "RNMetalRenderer.h"

namespace RN
{
	RNDefineMeta(MetalWindow, Window)

	MetalWindow::MetalWindow(const Vector2 &size, Screen *screen, MetalRenderer *renderer) :
		Window(screen),
		_renderer(nullptr)
	{
		_internals->metalView = [[RNMetalView alloc] initWithFrame:NSMakeRect(0, 0, size.x, size.y) andDevice:renderer->_internals->device];

		_internals->window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, size.x, size.y) styleMask:NSTitledWindowMask | NSResizableWindowMask | NSClosableWindowMask backing:NSBackingStoreBuffered defer:NO];
		[_internals->window setBackgroundColor:[NSColor blackColor]];
		[_internals->window setIgnoresMouseEvents:NO];
		[_internals->window setContentView:_internals->metalView];

		NSScreen *nsscreen = (NSScreen *)screen->GetNSScreen();
		NSRect frame = [_internals->window frame];

		CGFloat xPos = (NSWidth([nsscreen frame]) - NSWidth(frame)) * 0.5;
		CGFloat yPos = (NSHeight([nsscreen frame]) - NSHeight(frame)) * 0.5;
		[_internals->window setFrame:NSMakeRect(xPos, yPos, NSWidth(frame), NSHeight(frame)) display:YES];
	}

	void MetalWindow::SetTitle(const String *title)
	{
		[_internals->window setTitle:[NSString stringWithUTF8String:title->GetUTF8String()]];
	}

	Screen *MetalWindow::GetScreen()
	{
		Array *screens = Screen::GetScreens();
		NSScreen *nsscreen = [_internals->window screen];

		Screen *result = nullptr;

		screens->Enumerate<Screen>([&](Screen *screen, size_t index, bool &stop) {

			if(screen->GetNSScreen() == nsscreen)
			{
				result = screen;
				stop = true;
			}

		});

		RN_ASSERT(result, "Result must not be NULL, something broke internally");
		return result;
	}

	void MetalWindow::Show()
	{
		[_internals->window makeKeyAndOrderFront:nil];
	}

	void MetalWindow::Hide()
	{
		[_internals->window orderOut:nil];
	}

	Vector2 MetalWindow::GetSize() const
	{
		NSRect rect = [_internals->window frame];

		return Vector2(rect.size.width, rect.size.height);
	}
}
