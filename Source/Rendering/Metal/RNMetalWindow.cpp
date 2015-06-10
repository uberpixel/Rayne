//
//  RNMetalWindow.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../../Base/RNBaseInternal.h"
#include "RNMetalWindow.h"
#include "RNMetalInternals.h"
#include "RNMetalRenderer.h"

namespace RN
{
	MetalWindow::MetalWindow(const Rect &frame, Screen *screen, MetalRenderer *renderer) :
		Window(screen),
		_renderer(nullptr)
	{
		_internals->metalView = [[RNMetalView alloc] initWithFrame:NSMakeRect(0, 0, frame.width, frame.height) andDevice:renderer->_internals->device];

		_internals->window = [[NSWindow alloc] initWithContentRect:NSMakeRect(frame.x, frame.y, frame.width, frame.height) styleMask:NSTitledWindowMask | NSResizableWindowMask backing:NSBackingStoreBuffered defer:NO];
		[_internals->window setBackgroundColor:[NSColor blackColor]];
		[_internals->window setIgnoresMouseEvents:NO];
		[_internals->window setContentView:_internals->metalView];
		[_internals->window makeKeyAndOrderFront:nil];
	}

	void MetalWindow::SetTitle(const String *title)
	{
		[_internals->window setTitle:[NSString stringWithUTF8String:title->GetUTF8String()]];
	}
}
